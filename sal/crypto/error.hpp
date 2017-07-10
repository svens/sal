#pragma once

/**
 * \file sal/crypto/error.hpp
 */


#include <sal/config.hpp>
#include <sal/error.hpp>


__sal_begin


namespace crypto {


/**
 * Return reference to cryptography errors category. It's virtual method
 * name() returns string "crypto".
 */
const std::error_category &category () noexcept;


} // namespace crypto


__sal_end
