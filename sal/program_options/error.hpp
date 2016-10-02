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
using no_option_name = std::logic_error;

/**
 * Logic error when option with empty name is inserted into option_set_t
 */
using empty_option_name = std::logic_error;

/**
 * Logic error when option with invalid name is inserted into option_set_t
 */
using invalid_option_name = std::logic_error;

/**
 * Logic error when option with already existing name is inserted into
 * option_set_t
 */
using duplicate_option_name = std::logic_error;

/**
 * Runtime error when option requires argument but none is specified
 */
using option_requires_argument = std::runtime_error;

/**
 * Runtime error when option with no argument has argument
 */
using option_rejects_argument = std::runtime_error;


__sal_end
}} // namespace sal::program_options

/// \}
