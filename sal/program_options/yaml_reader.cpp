#include <sal/program_options/yaml_reader.hpp>
#include <sal/program_options/error.hpp>
#include <sal/assert.hpp>
#include <sal/fmt.hpp>
#include <cctype>
#include <deque>
#include <istream>
#include <limits>
#include <unordered_map>

//#define TRACE_PARSER
#if defined(TRACE_PARSER)
  #include <iostream>
#endif


namespace sal { namespace program_options {
__sal_begin


template <typename Arg>
bool trace (Arg &&arg)
{
#if defined(TRACE_PARSER)
  std::cout << arg;
#else
  (void)arg;
#endif
  return true;
}


template <typename... Args>
void trace (Args &&...args)
{
  bool unused[] = { trace(args)... };
  (void)unused;
}


namespace {

inline bool is_key_char (char ch) noexcept
{
  return std::isalnum(static_cast<int>(ch))
    || ch == '-'
    || ch == '_'
  ;
}

inline bool is_reference_char (char ch) noexcept
{
  return is_key_char(ch);
}

} // namespace


struct yaml_reader_t::impl_t
{
  std::istream &input;
  char look_ahead = '\0';
  size_t next_line = 1, next_column = 1;
  size_t line = 1, column = 1, base_column = 0;
  std::string *key{}, *value{};

  enum class quote_t
  {
    none,
    stop,
    any,
    one = any,
    two,
  } quote = quote_t::none;

  struct node_t
  {
    using stack_t = std::deque<node_t>;
    size_t column{};
    std::string key{};
    bool had_sub_node = false;
  };
  node_t::stack_t node_stack{};

  size_t list_column = 0;


  impl_t (std::istream &input)
    : input(input)
  {}


  impl_t (const impl_t &) = delete;
  impl_t &operator= (const impl_t &) = delete;


  // load next option/argument
  // returns true if has more, false otherwise
  bool next (std::string *option, std::string *argument);


  // read next character from input, updating position
  bool read (char &ch);


  // unread character: next read will return ch and not update position
  void unread (char ch)
  {
    look_ahead = ch;
  }


  // feed function return value:
  //  - false: current key/value pair is completed
  //  - true: keep reading input for current key/value pair
  bool (yaml_reader_t::impl_t::*feed)(char ch) = &impl_t::feed_node;
  bool feed_node (char ch);
  bool feed_key (char ch);
  bool feed_assign (char ch);
  bool feed_detect_value (char ch);
  bool feed_detect_next (char ch);
  bool feed_value (char ch);
  bool feed_list_item (char ch);
  bool feed_list (char ch);
  bool feed_reference (char ch);

  char get_and_escape ();
  bool finish_value (bool is_list_item);
  void strip_unquoted_value ();
  void update_reference ();

  std::unordered_map<std::string, std::string> references{};
  std::string reference{};
  bool make_reference = true;
  bool (impl_t::*context)(char ch) = &impl_t::feed_node;
  bool handle_reference (char ch, bool(impl_t::*return_context)(char));


  void throw_not_supported [[noreturn]] (const char *feature) const
  {
    throw_error<parser_error>(feature, " is not supported yet ",
      '(', line, ',', column, ')'
    );
  }


  void throw_expected_character [[noreturn]] (char ch) const
  {
    throw_error<parser_error>("expected character '", ch,
      "' (", line, ',', column, ')'
    );
  }


  void throw_unexpected_character [[noreturn]] () const
  {
    throw_error<parser_error>("unexpected character ",
      '(', line, ',', column, ')'
    );
  }


  void throw_bad_indent [[noreturn]] () const
  {
    throw_error<parser_error>("bad indent ",
      '(', line, ',', column, ')'
    );
  }
};


yaml_reader_t::yaml_reader_t (std::istream &input)
  : impl_(std::make_unique<impl_t>(input))
{}


yaml_reader_t::~yaml_reader_t () = default;


bool yaml_reader_t::operator() (const option_set_t &,
  std::string *option,
  std::string *argument)
{
  return impl_->next(option, argument);
}


bool yaml_reader_t::impl_t::read (char &ch)
{
  if (look_ahead != '\0')
  {
    ch = look_ahead;
    look_ahead = '\0';
    return true;
  }

  if (input.get(ch))
  {
    trace('\n', next_line, ',', next_column, '\t');
    line = next_line;
    column = next_column;

    if (ch == '\n')
    {
      trace("\\n");
    }
    else if (ch == ' ')
    {
      trace("sp");
    }
    else if (ch == '\t')
    {
      trace("\\t");
    }
    else
    {
      trace(ch);
    }
    trace('\t');

    if (ch == '#' && quote < quote_t::any)
    {
      input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      ch = '\n';
    }

    if (ch != '\n')
    {
      next_column++;
    }
    else
    {
      next_column = 1;
      next_line++;
    }

    return true;
  }

  return false;
}


bool yaml_reader_t::impl_t::next (std::string *option, std::string *argument)
{
  if (!input)
  {
    return false;
  }

  key = sal_check_ptr(option);
  value = sal_check_ptr(argument);

  char ch;
  while (read(ch))
  {
    if (!(this->*feed)(ch))
    {
      return true;
    }
  }

  if (feed == &impl_t::feed_list)
  {
    node_stack.clear();
  }

  if (node_stack.size() && !node_stack.back().had_sub_node)
  {
    finish_value(list_column != 0);
    return true;
  }

  return false;
}


void yaml_reader_t::impl_t::strip_unquoted_value ()
{
  if (quote != quote_t::none)
  {
    return;
  }

  while (value->size() && std::isblank(static_cast<int>(value->back())))
  {
    value->pop_back();
  }
}


void yaml_reader_t::impl_t::update_reference ()
{
  if (reference.empty())
  {
    return;
  }

  std::string ref;
  std::swap(ref, reference);

  if (make_reference)
  {
    trace("{+reference:", ref, " -> ", *value, '}');
    if (!references.emplace(ref, *value).second)
    {
      throw_error<parser_error>("duplicate reference: ", ref,
        " (on line ", line, ')'
      );
    }
    make_reference = false;
    return;
  }

  trace("{?reference:", ref, '}');
  if (value->size())
  {
    throw_error<parser_error>("trailing characters after reference",
      " (on line ", line, ')'
    );
  }

  auto it = references.find(ref);
  if (it == references.end())
  {
    throw_error<parser_error>("reference not found: ", ref,
      " (on line ", line, ')'
    );
  }
  *value = it->second;
}


bool yaml_reader_t::impl_t::finish_value (bool is_list_item)
{
  strip_unquoted_value();
  update_reference();

  for (const auto &node: node_stack)
  {
    key->append(node.key);
    key->push_back('.');
  }
  key->pop_back();

  trace("{finish:", *key, '=', *value, '}');

  if (is_list_item)
  {
    trace("{value -> list}");
    feed = &impl_t::feed_list;
  }
  else
  {
    // there is always at least one node, key itself
    node_stack.pop_back();
    trace("{value -> node}");
    feed = &impl_t::feed_node;
  }

  return false;
}


char yaml_reader_t::impl_t::get_and_escape ()
{
  auto ch = static_cast<char>(input.peek());
  switch (ch)
  {
    case 'a': ch = '\a'; break;
    case 'b': ch = '\b'; break;
    case 't': ch = '\t'; break;
    case 'n': ch = '\n'; break;
    case 'v': ch = '\v'; break;
    case 'f': ch = '\f'; break;
    case 'r': ch = '\r'; break;
    case '"': ch = '"'; break;
    case '/': ch = '/'; break;
    case '\\': ch = '\\'; break;
    default:
      throw_error<parser_error>("invalid escape \\", ch,
        " (", line, ',', column, ')'
      );
  }

  input.get();
  next_column++;

  return ch;
}


bool yaml_reader_t::impl_t::feed_node (char ch)
{
  if (ch == ' ' || ch == '\n')
  {
    return true;
  }
  else if (ch == '\t')
  {
    throw_unexpected_character();
  }

  if (!base_column)
  {
    base_column = column;
  }
  else if (column < base_column)
  {
    throw_bad_indent();
  }
  else
  {
    while (node_stack.size() && column <= node_stack.back().column)
    {
      node_stack.pop_back();
    }
  }

  if (node_stack.size())
  {
    node_stack.back().had_sub_node = true;
  }

  node_stack.emplace_back();
  node_stack.back().column = column;

  trace("(node -> key)");
  feed = &impl_t::feed_key;
  return feed_key(ch);
}


bool yaml_reader_t::impl_t::feed_key (char ch)
{
  if (is_key_char(ch))
  {
    node_stack.back().key.push_back(ch);
    return true;
  }

  if (node_stack.back().key.empty())
  {
    throw_unexpected_character();
  }

  trace("{key[", node_stack.back().column,
    ':', node_stack.back().key,
    "] -> assign)"
  );

  feed = &impl_t::feed_assign;
  return feed_assign(ch);
}


bool yaml_reader_t::impl_t::feed_assign (char ch)
{
  if (ch == ':')
  {
    trace("(assign -> detect_value)");
    feed = &impl_t::feed_detect_value;
  }
  else if (!std::isblank(static_cast<int>(ch)))
  {
    throw_expected_character(':');
  }
  return true;
}


bool yaml_reader_t::impl_t::feed_detect_value (char ch)
{
  if (ch == '|' || ch == '>')
  {
    throw_not_supported("multiline value");
  }
  else if (ch == '&' || ch == '*')
  {
    trace("(detect_value -> reference)");
    handle_reference(ch, &impl_t::feed_value);
  }
  else if (ch == '\n')
  {
    trace("(detect_value -> detect_next)");
    feed = &impl_t::feed_detect_next;
  }
  else if (!std::isblank(static_cast<int>(ch)))
  {
    trace("(detect_value -> value)");
    quote = quote_t::none;
    feed = &impl_t::feed_value;
    return feed_value(ch);
  }
  return true;
}


bool yaml_reader_t::impl_t::feed_detect_next (char ch)
{
  if (ch == ' ' || ch == '\n')
  {
    return true;
  }
  else if (ch == '\t')
  {
    throw_unexpected_character();
  }

  if (column > node_stack.back().column)
  {
    if (ch == '-')
    {
      trace("(detect_next -> list_item:", column, ')');
      list_column = column;
      feed = &impl_t::feed_list_item;
      return true;
    }
    else
    {
      trace("(detect_next -> node)");
      feed = &impl_t::feed_node;
      return feed_node(ch);
    }
  }

  unread(ch);
  return finish_value(false);
}


bool yaml_reader_t::impl_t::feed_list_item (char ch)
{
  if (ch == '\n')
  {
    return finish_value(true);
  }
  else if (value->empty())
  {
    if (std::isblank(static_cast<int>(ch)))
    {
      return true;
    }
    else if (ch == '|' || ch == '>')
    {
      throw_not_supported("multiline value");
    }
    else if (ch == '&' || ch == '*')
    {
      trace("(feed_list_item -> reference)");
      return handle_reference(ch, &impl_t::feed_list_item);
    }
  }

  value->push_back(ch);
  return true;
}


bool yaml_reader_t::impl_t::handle_reference (char ch,
  bool(impl_t::*return_context)(char))
{
  make_reference = ch == '&';
  context = return_context;
  feed = &impl_t::feed_reference;
  return true;
}


bool yaml_reader_t::impl_t::feed_list (char ch)
{
  if (ch == '-')
  {
    if (column == list_column)
    {
      trace("(list -> list_item)");
      feed = &impl_t::feed_list_item;
      return true;
    }
    throw_bad_indent();
  }
  else if (ch == ' ' || ch == '\n')
  {
    return true;
  }
  else if (ch == '\t')
  {
    throw_unexpected_character();
  }

  if (column > node_stack.back().column)
  {
    throw_bad_indent();
  }

  trace("(list -> node)");
  node_stack.pop_back();
  feed = &impl_t::feed_node;
  return feed_node(ch);
}


bool yaml_reader_t::impl_t::feed_value (char ch)
{
  if (ch == '\n' || quote == quote_t::stop)
  {
    if (ch == '\n' && quote < quote_t::any)
    {
      return finish_value(false);
    }
    else if (std::isblank(static_cast<int>(ch)))
    {
      return true;
    }
    throw_unexpected_character();
  }
  else if (ch == '\'' || ch == '"')
  {
    if (quote == quote_t::none)
    {
      trace("{+quote:", ch, '}');
      quote = ch == '\'' ? quote_t::one : quote_t::two;
      return true;
    }
    else if ((ch == '\'' && quote == quote_t::one)
      || (ch == '"' && quote == quote_t::two))
    {
      trace("{-quote}");
      quote = quote_t::stop;
      return true;
    }
  }
  else if (ch == '\\' && quote == quote_t::two)
  {
    ch = get_and_escape();
  }

  value->push_back(ch);
  return true;
}


bool yaml_reader_t::impl_t::feed_reference (char ch)
{
  if (is_reference_char(ch))
  {
    reference.push_back(ch);
  }
  else if (std::isspace(static_cast<int>(ch)))
  {
    trace("(reference[", reference, "] -> value)");
    feed = context;
    if (ch == '\n')
    {
      return (this->*feed)(ch);
    }
  }
  else
  {
    throw_unexpected_character();
  }
  return true;
}


__sal_end
}} // namespace sal::program_options
