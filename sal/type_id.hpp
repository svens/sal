#pragma once

/**
 * \file sal/type_id.hpp
 * Unique id per type without RTTI
 */

#include <sal/config.hpp>


__sal_begin


namespace __bits {

template <typename T>
const T *unique_address_for{};

} // namespace __bits


/**
 * Return unique id for T amongst all types.
 */
template <typename T>
inline constexpr uintptr_t type_id () noexcept
{
  return reinterpret_cast<uintptr_t>(&__bits::unique_address_for<T>);
}


/**
 * \copydoc type_id()
 */
template <typename T>
inline constexpr uintptr_t type_id (T) noexcept
{
  return reinterpret_cast<uintptr_t>(&__bits::unique_address_for<T>);
}


__sal_end
