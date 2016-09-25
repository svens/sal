#include <sal/program_options/option_set.hpp>
#include <cctype>
#include <iomanip>
#include <ostream>

#if __sal_os_windows
  #include <windows.h>
#else
  #include <sys/ioctl.h>
  #include <unistd.h>
#endif


namespace sal { namespace program_options {
__sal_begin


namespace {

size_t terminal_width ()
{
#if __sal_os_windows

  CONSOLE_SCREEN_BUFFER_INFO i;
  ::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &i);
  return i.srWindow.Right - i.srWindow.Left + 1;

#else

  winsize ws;
  ::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  return ws.ws_col;

#endif
}


inline std::string to_lower (std::string input) noexcept
{
  for (auto &ch: input)
  {
    ch = static_cast<char>(std::tolower(ch));
  }
  return input;
}


std::string to_str (const std::string &option,
  const __bits::option_t &settings)
{
  //
  // -X | --longX
  //

  std::string result = option.size() > 1 ? "--" : "-";
  result += option;

  if (settings.unit.empty())
  {
    return result;
  }

  //
  // '-X arg'
  // '--longX=arg'
  // '-X [arg]'
  // '--longX[=arg]'
  //

  if (option.size() > 1)
  {
    result += settings.requires_argument ? "" : "[";
    result += '=';
  }
  else
  {
    result += ' ';
    result += settings.requires_argument ? "" : "[";
  }

  result += settings.unit;

  if (!settings.requires_argument)
  {
    result += ']';
  }

  return result;
}


using display_list_t = std::map<
  std::string,          // sorting key
  std::pair<
    std::string,        // display string
    __bits::option_ptr  // option itself
  >
>;


struct name_info_t
{
  std::string sort_short{}, sort_long{};
  std::string display_short{}, display_long{};
};

using name_list_t = std::map<__bits::option_ptr, name_info_t>;


display_list_t sort_options (
  const std::map<std::string, __bits::option_ptr> &options,
  size_t *longest_display)
{
  // create reverse index (option_ptr -> short & long names)
  name_list_t name_list;
  for (const auto &it: options)
  {
    auto &name = name_list[it.second];
    if (it.first.size() == 1)
    {
      name.sort_short += it.first;
      name.sort_short += ' ';

      name.display_short += to_str(it.first, *it.second);
      name.display_short += ", ";
    }
    else
    {
      name.sort_long += it.first;
      name.sort_long += ' ';

      name.display_long += to_str(it.first, *it.second);
      name.display_long += ", ";
    }
  }

  //
  // create ordered display strings list
  //

  *longest_display = 0;

  display_list_t display_list;
  for (const auto &it: name_list)
  {
    auto &display = display_list[
      to_lower(it.second.sort_short + it.second.sort_long)
    ];

    display.first = it.second.display_short + it.second.display_long;
    display.first.erase(display.first.size() - 2, 2);

    if (display.first.size() > *longest_display)
    {
      *longest_display = display.first.size();
    }

    display.second = it.first;
  }

  return display_list;
}


template <typename F>
inline std::string::const_iterator skip (F skip_char,
  std::string::const_iterator first,
  std::string::const_iterator end) noexcept
{
  while (first != end && skip_char(*first))
  {
    ++first;
  }
  return first;
}


void print_help (std::ostream &os,
  const std::string &help,
  const std::string &indent,
  size_t width)
{
  auto is_space = [](int ch)
  {
    return std::isspace(ch);
  };
  auto is_non_space = [](int ch)
  {
    return !std::isspace(ch);
  };

  static const char nl_x1[] = "\n", nl_x2[] = "\n\n";
  const char *newline = nl_x1;

  auto remaining_width = width - indent.size();
  auto word = skip(is_space, help.begin(), help.end()), begin = word;

  while (word != help.end())
  {
    auto end_of_word = skip(is_non_space, word, help.end());
    size_t length = end_of_word - word;

    if (word != begin)
    {
      if (length + 1 < remaining_width)
      {
        // space between words
        os << ' ';
        --remaining_width;
      }
      else
      {
        // newline because current word wouldn't fit
        os << newline << indent;
        newline = nl_x1;
        remaining_width = width - indent.size();
      }
    }

    // word itself
    os << std::string(word, end_of_word);
    remaining_width -= length;

    // if ends with one or more '\n', force new line(s) (up to 2)
    // (newline itself will be added with next iteration)
    if (end_of_word != help.end() && *end_of_word == '\n')
    {
      remaining_width = 0;
      if (++end_of_word != help.end() && *end_of_word == '\n')
      {
        newline = nl_x2;
        ++end_of_word;
      }
    }

    word = skip(is_space, end_of_word, help.end());
  }
}


} // namespace


std::ostream &option_set_t::print (std::ostream &os, size_t width) const
{
  size_t longest_display;
  auto display_order = sort_options(options_, &longest_display);

  //
  // choose display format
  //  * longest_display > width/3: help wrapped to next line
  //  * else two columns
  //

  if (!width)
  {
    width = terminal_width();
  }

  std::string indent(4, ' ');
  bool two_column = longest_display < width / 3;
  if (two_column)
  {
    indent.append(longest_display, ' ');
  }

  //
  // actual printing in display_order
  //

  for (const auto &it: display_order)
  {
    const auto &display = it.second;

    os << "\n  ";
    if (two_column)
    {
      os << std::setw(longest_display + 2) << std::left << display.first;
      print_help(os, display.second->help, indent, width);
    }
    else
    {
      os << display.first << '\n' << indent;
      print_help(os, display.second->help, indent, width);
      os << '\n';
    }
  }

  return os;
}


bool option_set_t::valid_name (const std::string &name) const noexcept
{
  auto valid_name_char = [](int ch)
  {
    return std::isalnum(ch)
      || ch == '_'
      || ch == '.'
    ;
  };
  return skip(valid_name_char, name.begin(), name.end()) == name.end();
}


__sal_end
}} // namespace sal::program_options
