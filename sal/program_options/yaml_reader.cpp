#include <sal/program_options/yaml_reader.hpp>
#include <sal/program_options/error.hpp>
#include <sal/assert.hpp>
#include <cctype>
#include <istream>
#include <limits>

#include <iostream>
#include <iomanip>


namespace sal { namespace program_options {
__sal_begin


inline bool ends_with (const std::string &name, const std::string &suffix)
{
  if (suffix.size() <= name.size())
  {
    return std::equal(suffix.rbegin(), suffix.rend(), name.rbegin());
  }
  return false;
}


struct yaml_reader_t::impl_t
{
  std::istream &input;
  std::pair<size_t, size_t> input_pos{1, 1};
  std::string *option{}, *argument{};
  size_t argument_indent = 0;
  bool is_argument_line_indented = false;


  enum class style_t
  {
    unset,
    literal = '\n',
    folded = ' ',
  } style = style_t::unset;


  enum class chomp_t
  {
    clip,
    keep,
    strip,
  } chomp = chomp_t::clip;

  std::ostringstream oss;
  std::streambuf *old_buf;

  impl_t (std::istream &input)
    : input(input)
    , oss()
    , old_buf(std::cout.rdbuf())
  {
    std::cout.rdbuf(oss.rdbuf());
  }


  ~impl_t ()
  {
    std::cout.rdbuf(old_buf);
  }


  impl_t (const impl_t &) = delete;
  impl_t &operator= (const impl_t &) = delete;


  // load next option/argument
  // returns true if has more, false otherwise
  bool next (std::string *option, std::string *argument);

  // ignore current line until end of line
  void ignore_until_eol ()
  {
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    input_pos.first++;
    input_pos.second = 0;

  }


  // state function return value:
  //  - false: current option/argument pair is completed
  //  - true: keep reading input for current option/argument pair
  bool (yaml_reader_t::impl_t::*state)(char ch) = &impl_t::handle_start;
  bool handle_start (char ch);
  bool handle_option (char ch);
  bool handle_assign (char ch);
  bool handle_assign_settings (char ch);
  bool handle_argument (char ch);
  bool handle_argument_eol (char ch);


  void handle_trail ()
  {
    std::cout << "(handle_trail:";
    if (style == style_t::unset || chomp == chomp_t::strip)
    {
      std::cout << "unset|strip";
      while (argument->size()
        && std::isspace(static_cast<int>(argument->back())))
      {
        argument->pop_back();
      }
    }
    else if (chomp == chomp_t::clip)
    {
      std::cout << "clip";
      static const std::string newline = "\n", double_newline = "\n\n";
      while (*argument == newline || ends_with(*argument, double_newline))
      {
        argument->pop_back();
      }
    }
    std::cout << ')' << std::endl;
  }


  void unexpected_tab [[noreturn]] () const
  {
    throw_error<parser_error>("unexpected TAB at ",
      '(', input_pos.first, ',', input_pos.second, ')'
    );
  }


  void expected_character [[noreturn]] (char ch) const
  {
    throw_error<parser_error>("expected character '", ch,
      "' at (", input_pos.first, ',', input_pos.second, ')'
    );
  }


  void expected_newline [[noreturn]] () const
  {
    throw_error<parser_error>("expected newline at ",
      '(', input_pos.first, ',', input_pos.second, ')'
    );
  }


  void bad_indent [[noreturn]] () const
  {
    throw_error<parser_error>("bad indent at ",
      '(', input_pos.first, ',', input_pos.second, ')'
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


namespace {

inline bool is_valid_option_name_char (char ch) noexcept
{
  return std::isalnum(static_cast<int>(ch))
    || ch == '-'
    || ch == '.'
    || ch == '/'
    || ch == '_'
  ;
}

} // namespace


bool yaml_reader_t::impl_t::next (std::string *opt, std::string *arg)
{
  if (!input)
  {
    return false;
  }

  option = sal_check_ptr(opt);
  argument = sal_check_ptr(arg);

  char ch;
  while (input.get(ch))
  {
    std::cout << "\n[" << input_pos.first << ',' << input_pos.second << "] ";
    if (ch == ' ')
    {
      std::cout << "' '";
    }
    else if (ch == '\n')
    {
      std::cout << "\\n";
    }
    else if (ch == '\t')
    {
      std::cout << "\\t";
    }
    else if (std::isprint(static_cast<int>(ch)))
    {
      std::cout << ch;
    }
    std::cout << ' ' << std::setw(5) << '\t';

    if (!(this->*state)(ch))
    {
      handle_trail();
      return true;
    }

    if (ch == '\n')
    {
      input_pos.first++;
      input_pos.second = 0;
    }
    input_pos.second++;
  }

  handle_trail();
  return option->size() > 0;
}


bool yaml_reader_t::impl_t::handle_start (char ch)
{
  if (ch == '#')
  {
    ignore_until_eol();
  }
  else if (ch == '\t')
  {
    unexpected_tab();
  }
  else if (ch != ' ' && ch != '\n')
  {
    if (input_pos.second == 1)
    {
      std::cout << "(start -> option)";
      state = &impl_t::handle_option;
      return handle_option(ch);
    }
    bad_indent();
  }

  return true;
}


bool yaml_reader_t::impl_t::handle_option (char ch)
{
  if (is_valid_option_name_char(ch))
  {
    option->push_back(ch);
    return true;
  }

  std::cout << "(option -> assign)";
  state = &impl_t::handle_assign;
  return handle_assign(ch);
}


bool yaml_reader_t::impl_t::handle_assign (char ch)
{
  if (ch == ':')
  {
    style = style_t::unset;
    chomp = chomp_t::clip;
    state = &impl_t::handle_assign_settings;
  }
  else if (!std::isblank(ch))
  {
    expected_character(':');
  }

  return true;
}


bool yaml_reader_t::impl_t::handle_assign_settings (char ch)
{
  if (ch == ':')
  {
    expected_newline();
  }
  else if (ch == '|' || ch == '>')
  {
    if (style != style_t::unset)
    {
      std::cout << "(expected_newline)";
      expected_newline();
    }

    style = ch == '|' ? style_t::literal : style_t::folded;
    std::cout << "(style:" << (style == style_t::literal ? "literal" : "folded") << ')';

    ch = static_cast<char>(input.peek());
    if (ch == '+' || ch == '-')
    {
      input.get();
      input_pos.second++;
      chomp = ch == '+' ? chomp_t::keep : chomp_t::strip;
      std::cout << "(chomp:" << (chomp == chomp_t::keep ? "keep" : "strip") << ')';
    }
  }
  else if (ch == '\n' || ch == '#')
  {
    if (ch == '#')
    {
      ignore_until_eol();
    }
    std::cout << "(assign -> argument_eol)";
    state = &impl_t::handle_argument_eol;
    is_argument_line_indented = false;
    argument_indent = 0;
  }
  else if (!std::isblank(ch))
  {
    if (style == style_t::unset)
    {
      std::cout << "(assign -> argument)";
      state = &impl_t::handle_argument;
      return handle_argument(ch);
    }
    expected_newline();
  }

  return true;
}


bool yaml_reader_t::impl_t::handle_argument (char ch)
{
  if (ch == '#' && style == style_t::unset)
  {
    ignore_until_eol();
    std::cout << "(argument -> start)";
    state = &impl_t::handle_start;
    return false;
  }
  else if (ch == '\n')
  {
    if (style != style_t::unset)
    {
      std::cout << "(+literal)";
      argument->push_back(ch);
    }
    std::cout << "(argument -> argument_eol)";
    state = &impl_t::handle_argument_eol;
  }
  else
  {
    argument->push_back(ch);
  }

  return true;
}


bool yaml_reader_t::impl_t::handle_argument_eol (char ch)
{
  if (ch == '\n')
  {
    if (style != style_t::unset || argument->size())
    {
      std::cout << "(+newline)";
      argument->push_back(ch);
    }
    return true;
  }
  else if (ch == ' ')
  {
    if (input_pos.second == argument_indent)
    {
      argument->push_back(ch);
      is_argument_line_indented = true;
      std::cout << "(argument_eol -> argument)";
      state = &impl_t::handle_argument;
    }
    return true;
  }
  else if (ch == '\t')
  {
    unexpected_tab();
  }

  if (input_pos.second == 1)
  {
    input.unget();
    std::cout << "(argument_eol -> start)";
    state = &impl_t::handle_start;
    return false;
  }

  if (argument->size())
  {
    if (style != style_t::unset)
    {
      argument->back() = is_argument_line_indented ? '\n' : static_cast<char>(style);
      is_argument_line_indented = false;
    }
    else if (argument->back() != '\n')
    {
      argument->push_back(' ');
    }
  }

  if (!argument_indent && style != style_t::unset)
  {
    argument_indent = input_pos.second;
    std::cout << "(indent=" << argument_indent << ')';
  }

  std::cout << "(argument_eol -> argument)";
  state = &impl_t::handle_argument;
  return handle_argument(ch);
}


__sal_end
}} // namespace sal::program_options
