#include <sal/program_options/config_reader.hpp>
#include <sal/program_options/error.hpp>
#include <sal/assert.hpp>
#include <cctype>
#include <deque>
#include <istream>


namespace sal { namespace program_options {
__sal_begin


namespace {

constexpr bool is_json_punct (int ch)
{
  return ch == ','
    || ch == ':'
    || ch == '='
    || ch == '['
    || ch == ']'
    || ch == '{'
    || ch == '}'
  ;
}

constexpr bool is_bare_key_char (int it)
{
  return it == '-'
    || it == '_'
    || (it >= 'A' && it <= 'Z')
    || (it >= 'a' && it <= 'z')
    || (it >= '0' && it <= '9')
  ;

}

} // namespace


struct config_reader_t::impl_t
{
  std::istream &input;

  std::string cache{};
  std::string::const_iterator cache_it = cache.end();
  size_t line = 0;
  int it = 0;


  struct node_t
  {
    using stack_t = std::deque<node_t>;

    bool(impl_t::*context)(node_t &) = &impl_t::any;
    std::string key{};
    bool allow_comma = false;

    node_t () = default;

    node_t (bool(impl_t::*context)(node_t &))
      : context(context)
    {}

    node_t (bool(impl_t::*context)(node_t &), const std::string &key)
      : context(context)
      , key(key)
    {}

    void set (bool(impl_t::*to)(node_t &))
    {
      context = to;
    }
  };
  node_t::stack_t objects{};
  std::string current_value{};


  impl_t (std::istream &input);

  impl_t (const impl_t &) = delete;
  impl_t &operator= (const impl_t &) = delete;


  // extract option/argument
  // returns true if has more, false otherwise
  bool extract (std::string *option, std::string *argument);
  bool key_and_value (std::string *option, std::string *argument);


  // return current column number
  size_t column () const
  {
    return cache_it - cache.begin();
  }


  // read next character, caching line if necessary
  bool next ()
  {
    if (cache_it != cache.end() || load_cache())
    {
      it = *cache_it++;
    }
    else
    {
      it = 0;
    }
    return it != 0;
  }


  bool load_cache ();


  // peek at following char
  int peek () const noexcept
  {
    return *cache_it;
  }


  int unescape (int ch) const
  {
    switch (ch)
    {
      case '"': return '"';
      case '\\': return '\\';
      case 'b': return '\b';
      case 'f': return '\f';
      case 'n': return '\n';
      case 'r': return '\r';
      case 't': return '\t';
    }
    throw_unexpected("escaped character");
  }


  //
  // state handlers
  //

  bool any (node_t &node);
  bool object (node_t &node);
  bool array (node_t &node);
  bool assign (node_t &node);
  bool value (node_t &node);



  //
  // token extractors
  //

  bool skip_spaces_and_comments ();

  // different kind of extractors
  std::string extract_string (bool is_key);
  std::string extract_bare_key ();
  std::string extract_quoteless_string ();
  std::string extract_basic_string (bool allow_multiline);
  std::string extract_literal_string (bool allow_multiline);
  std::string extract_basic_multiline_string ();
  std::string extract_literal_multiline_string ();


  //
  // errors
  //

  void throw_not_supported [[noreturn]] (const char *what) const
  {
    throw_error<parser_error>(what, " is not supported ",
      '(', line, ',', column(), ')'
    );
  }


  void throw_expected [[noreturn]] (const char *what) const
  {
    throw_error<parser_error>("expected ", what, ' ',
      '(', line, ',', column(), ')'
    );
  }


  void throw_unexpected [[noreturn]] (const char *what) const
  {
    throw_error<parser_error>("unexpected ", what, ' ',
      '(', line, ',', column(), ')'
    );
  }
};


config_reader_t::config_reader_t (std::istream &input)
  : impl_(std::make_unique<impl_t>(input))
{}


config_reader_t::~config_reader_t () = default;


bool config_reader_t::operator() (const option_set_t &,
  std::string *option,
  std::string *argument)
{
  return impl_->extract(option, argument);
}


config_reader_t::impl_t::impl_t (std::istream &input)
  : input(input)
{
  objects.emplace_back();
  if (next() && skip_spaces_and_comments() && it == '{')
  {
    object(objects.back());
  }
}


bool config_reader_t::impl_t::extract (std::string *option,
  std::string *argument)
{
  while (skip_spaces_and_comments())
  {
    if (objects.empty())
    {
      throw_unexpected("end of object");
    }
    auto &node = objects.back();
    if (!(this->*node.context)(node))
    {
      return key_and_value(option, argument);
    }
  }

  while (objects.size())
  {
    const auto &node = objects.back();
    if (node.context == &impl_t::value)
    {
      return key_and_value(option, argument);
    }
    else if (node.context != &impl_t::any)
    {
      throw_unexpected("end of input");
    }
    objects.pop_back();
  }

  return false;
}


bool config_reader_t::impl_t::key_and_value (std::string *option,
  std::string *argument)
{
  std::string key;
  for (const auto &object: objects)
  {
    if (object.key.size()
      && (object.context == &impl_t::object
        || object.context == &impl_t::array
        || object.context == &impl_t::value))
    {
      key += object.key;
      key += '.';
    }
  }
  if (key.size() && key.back() == '.')
  {
    key.pop_back();
  }

  *sal_check_ptr(argument) = std::move(current_value);
  *sal_check_ptr(option) = std::move(key);

  auto &back = objects.back();
  if (back.context != &impl_t::array)
  {
    back.set(&impl_t::any);
  }
  back.allow_comma = true;
  return true;
}


bool config_reader_t::impl_t::any (node_t &node)
{
  if (it == '}')
  {
    objects.pop_back();
    return true;
  }
  else if (it == ',')
  {
    if (!node.allow_comma)
    {
      throw_unexpected("comma");
    }
    node.allow_comma = false;
  }
  else if (it == '"' || it == '\'' || is_bare_key_char(it))
  {
    node.key = extract_string(true);
    node.set(&impl_t::assign);
    return true;
  }
  else
  {
    throw_unexpected("character");
  }
  return next();
}


bool config_reader_t::impl_t::object (node_t &node)
{
  if (it == '{')
  {
    node.set(&impl_t::object);
    objects.emplace_back();
    return next();
  }
  else if (it == '}')
  {
    objects.pop_back();
    next();
    if (objects.size())
    {
      return true;
    }
  }

  objects.emplace_back();
  objects.back().allow_comma = true;
  return true;
}


bool config_reader_t::impl_t::array (node_t &node)
{
  if (it == ']')
  {
    objects.back().set(&impl_t::any);
    return next();
  }
  else if (it == ',')
  {
    if (!node.allow_comma)
    {
      throw_unexpected("comma");
    }
    node.allow_comma = false;
    return next();
  }
  else if (it == '[' || it == '{')
  {
    throw_not_supported("array of arrays or objects");
  }
  else if (is_json_punct(it))
  {
    throw_unexpected("character");
  }
  current_value = extract_string(false);
  node.allow_comma = true;
  return false;
}


bool config_reader_t::impl_t::assign (node_t &node)
{
  if (it == '=' || it == ':')
  {
    node.set(&impl_t::value);
    return next();
  }
  throw_expected("':' or '='");
}


bool config_reader_t::impl_t::value (node_t &node)
{
  if (it == '{')
  {
    node.set(&impl_t::object);
    objects.emplace_back();
    return next();
  }
  else if (it == '[')
  {
    node.set(&impl_t::array);
    return next();
  }
  current_value = extract_string(false);
  return false;
}


std::string config_reader_t::impl_t::extract_string (bool is_key)
{
  if (it == '"')
  {
    return extract_basic_string(is_key == false);
  }
  else if (it == '\'')
  {
    return extract_literal_string(is_key == false);
  }
  else
  {
    return is_key ? extract_bare_key() : extract_quoteless_string();
  }
}


std::string config_reader_t::impl_t::extract_bare_key ()
{
  std::string result;
  while (is_bare_key_char(it))
  {
    result += static_cast<char>(it);
    next();
  }

  return result;
}


std::string config_reader_t::impl_t::extract_quoteless_string ()
{
  std::string result;
  while (!is_json_punct(it) && !std::isspace(it))
  {
    if (it == '/')
    {
      auto ch = peek();
      if (ch == '/' || ch == '*')
      {
        break;
      }
    }
    result += static_cast<char>(it);
    next();
  }

  return result;
}


std::string config_reader_t::impl_t::extract_basic_string (bool allow_multiline)
{
  next();
  if (allow_multiline && it == '"' && peek() == '"')
  {
    next(), next();
    return extract_basic_multiline_string();
  }

  std::string result;
  while (it != '"')
  {
    if (it == '\n')
    {
      throw_unexpected("newline");
    }
    else if (it == '\\')
    {
      next();
      it = unescape(it);
    }
    result += static_cast<char>(it);
    next();
  }
  next();

  return result;
}


std::string config_reader_t::impl_t::extract_literal_string (bool allow_multiline)
{
  next();
  if (allow_multiline && it == '\'' && peek() == '\'')
  {
    next(), next();
    return extract_literal_multiline_string();
  }

  std::string result;
  while (it != '\'')
  {
    if (it == '\n')
    {
      throw_unexpected("newline");
    }
    result += static_cast<char>(it);
    next();
  }
  next();

  return result;
}


std::string config_reader_t::impl_t::extract_basic_multiline_string ()
{
  std::string result;

  if (it == '\n')
  {
    next();
  }

  bool skip_whitespaces = false;
  for (auto consecutive_quotes = 0U;  it && consecutive_quotes != 3;  next())
  {
    if (skip_whitespaces && std::isspace(it))
    {
      continue;
    }
    skip_whitespaces = false;

    result += static_cast<char>(it);

    if (it == '"')
    {
      ++consecutive_quotes;
      continue;
    }
    consecutive_quotes = 0;

    if (it == '\\')
    {
      next();
      if (it != '\n')
      {
        result.back() = static_cast<char>(unescape(it));
      }
      else
      {
        result.pop_back();
        skip_whitespaces = true;
      }
    }
  }

  if (it)
  {
    result.erase(result.size() - 3);
  }
  else
  {
    throw_unexpected("end of input");
  }

  return result;
}


std::string config_reader_t::impl_t::extract_literal_multiline_string ()
{
  std::string result;

  if (it == '\n')
  {
    next();
  }

  for (auto consecutive_quotes = 0U;  it && consecutive_quotes != 3;  next())
  {
    result += static_cast<char>(it);
    if (it != '\'')
    {
      consecutive_quotes = 0;
    }
    else
    {
      ++consecutive_quotes;
    }
  }

  if (it)
  {
    result.erase(result.size() - 3);
  }
  else
  {
    throw_unexpected("end of input");
  }

  return result;
}


bool config_reader_t::impl_t::load_cache ()
{
  if (!std::getline(input, cache))
  {
    return false;
  }

  while (cache.size()
    && std::isspace(static_cast<int>(cache.back())))
  {
    cache.pop_back();
  }

  line++;
  cache.push_back('\n');
  cache_it = cache.begin();

  return true;
}


bool config_reader_t::impl_t::skip_spaces_and_comments ()
{
  auto comment_recursion_depth = 0U;

  do
  {
    if (!it)
    {
      break;
    }
    else if (it == '/' && peek() == '*')
    {
      comment_recursion_depth++;
      next();
    }
    else if (it == '*' && peek() == '/')
    {
      comment_recursion_depth--;
      next();
    }
    else if (!comment_recursion_depth)
    {
      if (it == '/' && peek() == '/')
      {
        cache_it = cache.end();
      }
      else if (!std::isspace(it))
      {
        return true;
      }
    }
  } while (next());

  if (comment_recursion_depth)
  {
    throw_unexpected("end of input");
  }

  return false;
}


__sal_end
}} // namespace sal::program_options
