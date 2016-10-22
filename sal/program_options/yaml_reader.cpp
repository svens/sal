#include <sal/program_options/yaml_reader.hpp>
#include <sal/program_options/error.hpp>
#include <sal/assert.hpp>
#include <sal/fmt.hpp>
#include <cctype>
#include <deque>
#include <istream>
#include <limits>
#include <unordered_map>

#include <iostream>
#include <iomanip>


namespace sal { namespace program_options {
__sal_begin


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
  size_t line = 1, column = 1;
  size_t current_line = 1, current_column = 1;
  std::string *key{}, *value{};

  enum class quote_t
  {
    none,
    stop,
    any,
    one = any,
    two,
  } quote = quote_t::none;

  size_t root_node_column = 0;
  std::deque<std::pair<size_t, std::string>> node_stack{};

  std::unordered_map<std::string, std::string> references{};
  std::string reference{};
  bool make_reference = true;


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


  // feed function return value:
  //  - false: current key/value pair is completed
  //  - true: keep reading input for current key/value pair
  bool (yaml_reader_t::impl_t::*feed)(char ch) = &impl_t::feed_node;
  bool feed_node (char ch);
  bool feed_key (char ch);
  bool feed_assign (char ch);
  bool feed_detect_node_or_value (char ch);
  bool feed_value (char ch);
  bool feed_reference (char ch);

  char get_and_escape ();
  bool finish_value ();
  void strip_unquoted_value ();
  void update_reference ();


  void throw_not_supported [[noreturn]] (const char *feature) const
  {
    throw_error<parser_error>(feature, " is not supported yet ",
      '(', current_line, ',', current_column, ')'
    );
  }


  void throw_expected_character [[noreturn]] (char ch) const
  {
    throw_error<parser_error>("expected character '", ch,
      "' (", current_line, ',', current_column, ')'
    );
  }


  void throw_unexpected_character [[noreturn]] () const
  {
    throw_error<parser_error>("unexpected character ",
      '(', current_line, ',', current_column, ')'
    );
  }


  void throw_bad_indent [[noreturn]] () const
  {
    throw_error<parser_error>("bad indent ",
      '(', current_line, ',', current_column, ')'
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
  if (input.get(ch))
  {
    std::cout << '\n' << line << ',' << column << '\t';
    current_line = line;
    current_column = column;

    if (ch == '\n')
    {
      std::cout << "\\n";
    }
    else if (ch == ' ')
    {
      std::cout << "sp";
    }
    else if (ch == '\t')
    {
      std::cout << "\\t";
    }
    else
    {
      std::cout << ch;
    }
    std::cout << '\t';

    if (ch == '#' && quote < quote_t::any)
    {
      input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      ch = '\n';
    }

    if (ch != '\n')
    {
      column++;
    }
    else
    {
      column = 1;
      line++;
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

  if (node_stack.size())
  {
    finish_value();
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

  while (value->size() && std::isblank(value->back()))
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
    std::cout << "{+reference: " << ref << " -> " << *value << '}';
    if (!references.emplace(ref, *value).second)
    {
      throw_error<parser_error>("duplicate reference: ", ref,
        " (on line ", current_line, ')'
      );
    }
    make_reference = false;
    return;
  }

  std::cout << "{?reference: " << ref << '}';
  if (value->size())
  {
    throw_error<parser_error>("trailing characters after reference",
      " (on line ", current_line, ')'
    );
  }

  auto it = references.find(ref);
  if (it == references.end())
  {
    throw_error<parser_error>("reference not found: ", ref,
      " (on line ", current_line, ')'
    );
  }
  *value = it->second;
}


bool yaml_reader_t::impl_t::finish_value ()
{
  strip_unquoted_value();
  update_reference();

  for (const auto &node: node_stack)
  {
    key->append(node.second);
    key->push_back('.');
  }
  key->pop_back();

  // there is always at least one node, key itself
  node_stack.pop_back();

  std::cout << "{finish: " << *key << "=" << *value << '}';
  std::cout << "(value -> node)";
  feed = &impl_t::feed_node;
  return false;
}


char yaml_reader_t::impl_t::get_and_escape ()
{
  auto ch = input.peek();
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
      throw_error<parser_error>("invalid escape \\",
        static_cast<char>(ch),
        " (", current_line, ',', current_column, ')'
      );
  }

  input.get();
  column++;

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

  if (!root_node_column)
  {
    root_node_column = column;
  }
  if (column < root_node_column)
  {
    throw_bad_indent();
  }

  node_stack.emplace_back(column - 1, "");

  std::cout << "(node -> key)";
  feed = &impl_t::feed_key;
  return feed_key(ch);
}


bool yaml_reader_t::impl_t::feed_key (char ch)
{
  if (is_key_char(ch))
  {
    node_stack.back().second.push_back(ch);
    return true;
  }

  if (node_stack.back().second.empty())
  {
    throw_unexpected_character();
  }

  std::cout << "(key[" << node_stack.back().first
    << ':' << node_stack.back().second
    << "] -> assign)";
  feed = &impl_t::feed_assign;
  return feed_assign(ch);
}


bool yaml_reader_t::impl_t::feed_assign (char ch)
{
  if (ch == ':')
  {
    std::cout << "(assign -> detect_node_or_value)";
    feed = &impl_t::feed_detect_node_or_value;
  }
  else if (!std::isblank(ch))
  {
    throw_expected_character(':');
  }
  return true;
}


bool yaml_reader_t::impl_t::feed_detect_node_or_value (char ch)
{
  if (ch == '|' || ch == '>')
  {
    throw_not_supported("multiline value");
  }
  else if (ch == '&' || ch == '*')
  {
    make_reference = ch == '&';
    std::cout << "(detect_node_or_value -> reference)";
    feed = &impl_t::feed_reference;
  }
  else if (ch == '\n')
  {
    // TODO(nesting)
    if (input.peek() == ' ')
    {
      std::cout << "(detect_node_or_value -> node)";
      feed = &impl_t::feed_node;
      return true;
    }
    return finish_value();
  }
  else if (!std::isblank(ch))
  {
    std::cout << "(detect_node_or_value -> value)";
    quote = quote_t::none;
    feed = &impl_t::feed_value;
    return feed_value(ch);
  }
  return true;
}


bool yaml_reader_t::impl_t::feed_value (char ch)
{
  if (ch == '\n' || quote == quote_t::stop)
  {
    if (ch == '\n' && quote < quote_t::any)
    {
      return finish_value();
    }
    else if (std::isblank(ch))
    {
      return true;
    }
    throw_unexpected_character();
  }
  else if (ch == '\'' || ch == '"')
  {
    if (quote == quote_t::none)
    {
      std::cout << "{+quote:" << ch << '}';
      quote = ch == '\'' ? quote_t::one : quote_t::two;
      return true;
    }
    else if ((ch == '\'' && quote == quote_t::one)
      || (ch == '"' && quote == quote_t::two))
    {
      std::cout << "{-quote}";
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
  else if (std::isspace(ch))
  {
    std::cout << "(reference[" << reference << "] -> value)";
    feed = &impl_t::feed_value;
    if (ch == '\n')
    {
      return feed_value(ch);
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
