/// \addtogroup program_options
#pragma once

/**
 * \file sal/program_options/error.hpp
 */


#include <sal/config.hpp>
#include <sal/error.hpp>


namespace sal { namespace program_options {
__sal_begin


/**
 * Logic error when option with no name is inserted into option_set_t
 */
struct no_option_name_error
  : public std::logic_error
{
  using std::logic_error::logic_error;
};


/**
 * Logic error when option with empty name is inserted into option_set_t
 */
struct empty_option_name_error
  : public std::logic_error
{
  using std::logic_error::logic_error;
};


/**
 * Logic error when option with invalid name is inserted into option_set_t
 */
struct invalid_option_name_error
  : public std::logic_error
{
  using std::logic_error::logic_error;
};


/**
 * Logic error when option with already existing name is inserted into
 * option_set_t
 */
struct duplicate_option_name_error
  : public std::logic_error
{
  using std::logic_error::logic_error;
};


/**
 * Runtime error when parsed option that is not registered in option_set_t
 */
struct unknown_option_error
  : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};


/**
 * Runtime error when option requires argument but none is specified
 */
struct option_requires_argument_error
  : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};


/**
 * Runtime error when option with no argument has argument
 */
struct option_rejects_argument_error
  : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};


/**
 * Runtime error when parser fails
 */
struct parser_error
  : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};


__sal_end
}} // namespace sal::program_options

/// \}
