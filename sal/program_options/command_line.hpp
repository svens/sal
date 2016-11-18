/// \addtogroup program_options
#pragma once

/**
 * \file sal/program_options/command_line.hpp
 * GNU-style command line options parser
 */


#include <sal/config.hpp>
#include <sal/program_options/option_set.hpp>


namespace sal { namespace program_options {
__sal_begin


/**
 * GNU-style command line options parser for option_set_t::load_from().
 */
class command_line_t
{
public:

  /**
   * Construct parser using \a argc options from \a argv. This class does not
   * make deep-copy of \a argv but uses pointer directly.
   *
   * \note \a argv[0] is assumed to be application name and is ignored during
   * parsing.
   */
  command_line_t (int argc, const char **argv) noexcept
    : it_(argv + 1)
    , end_(argv + argc)
    , next_(it_ != end_ ? it_[0] : nullptr)
  {}


  /**
   * Get next option/argument pair from \a argv specified in constructor.
   * \see option_set_t::load_from()
   */
  bool operator() (const option_set_t &option_set,
    std::string *option,
    std::string *argument
  );


private:

  const char **it_;
  const char ** const end_;
  const char *next_;

  enum class state_t { undef, option, argument, stopped } state_ = state_t::undef;

  bool peek (char *p=nullptr) const noexcept
  {
    if (next_ != nullptr)
    {
      if (p)
      {
        *p = *next_;
      }
      return true;
    }
    return false;
  }

  char pop () noexcept
  {
    auto ch = *next_++;
    if (!ch)
    {
      next_ = ++it_ != end_ ? it_[0] : nullptr;
    }
    return ch;
  }
};


__sal_end
}} // namespace sal::program_options

/// \}
