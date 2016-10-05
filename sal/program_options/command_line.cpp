#include <sal/program_options/command_line.hpp>
#include <sal/assert.hpp>

#include <iostream>


namespace sal { namespace program_options {
__sal_begin


bool command_line_t::operator() (const option_set_t &option_set,
  std::string *option,
  std::string *argument)
{
  sal_check_ptr(option)->clear();
  sal_check_ptr(argument)->clear();

  char ch = '\0';
  bool is_long = false;

  // undef -> { option | argument | stopped }
  while (state_ == state_t::undef && peek())
  {
    if ((ch = pop()) == '\0')
    {
      // empty positional parameter
      return true;
    }

    if (ch != '-')
    {
      // first character of positional argument
      state_ = state_t::argument;
      *argument = ch;
      break;
    }

    peek(&ch);
    if (!ch)
    {
      // '-' in middle
      pop();
      continue;
    }
    else if (ch != '-')
    {
      // '-o'
      state_ = state_t::option;
      break;
    }
    pop();

    peek(&ch);
    if (!ch)
    {
      // '--' in middle
      pop();
      state_ = state_t::stopped;
      break;
    }

    // '--o'
    state_ = state_t::option;
    is_long = true;
  }

  // option (short: single character, long: until space or '=')
  while (state_ == state_t::option
    && (ch = pop()) != '\0'
    && ch != '=')
  {
    *option += ch;
    if (!is_long)
    {
      break;
    }
  }

  // option -> { option | argument | undef }
  while (state_ == state_t::option)
  {
    auto option_p = option_set.find(*option);

    if (!option_p)
    {
      throw_error<unknown_option>("unknown option: ", *option);
    }

    if (option_p->no_argument())
    {
      if (ch == '=')
      {
        // must not have argument but still has
        state_ = state_t::argument;
        throw_error<option_rejects_argument>(
          "option rejects argument: ", *option
        );
      }
      if (!ch || !peek(&ch) || (!ch && !pop()))
      {
        // at the end of command line or current option
        state_ = state_t::undef;
      }
      return true;
    }

    // with possible argument
    if (peek(&ch))
    {
      if (!ch)
      {
        // at the end of current option
        pop();
        if (!peek(&ch))
        {
          // ... and also command line
          ch = '\0';
        }
      }

      if (ch && ch != '-')
      {
        state_ = state_t::argument;
        break;
      }
    }

    state_ = state_t::undef;
    if (option_p->requires_argument())
    {
      throw_error<option_requires_argument>(
        "option requires argument: ", *option
      );
    }

    return true;
  }

  // argument | stopped -> { undef | stopped }
  while ((state_ == state_t::argument || state_ == state_t::stopped)
    && peek())
  {
    if ((ch = pop()) == '\0')
    {
      if (state_ == state_t::argument)
      {
        state_ = state_t::undef;
      }
      return true;
    }
    *argument += ch;
  }

  return false;
}


__sal_end
}} // namespace sal::program_options
