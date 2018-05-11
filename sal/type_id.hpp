#pragma once

/**
 * \file sal/type_id.hpp
 * Unique id per type without RTTI
 */

#include <sal/config.hpp>


__sal_begin


namespace __bits {


constexpr uint64_t fnv_1a_32 (const char *p) noexcept
{
  uint64_t h = 0x811c9dc5;
  while (*p)
  {
    h ^= *p++;
    h += static_cast<uint32_t>((h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24));
  }
  return h;
}


template <typename T>
struct type_t
{
  static constexpr uint64_t id () noexcept
  {
#if defined(__GNUC__)
    return fnv_1a_32(__PRETTY_FUNCTION__);
#elif defined(_MSC_VER)
    return fnv_1a_32(__FUNCSIG__);
#endif
  }
};


} // namespace __bits


/**
 * Return unique id for T amongst all types.
 */
template <typename T>
inline uintptr_t type_id () noexcept
{
  return reinterpret_cast<uintptr_t>(&__bits::type_t<T>::id);
}


/**
 * \copydoc type_id()
 */
template <typename T>
inline uintptr_t type_id (T) noexcept
{
  return reinterpret_cast<uintptr_t>(&__bits::type_t<T>::id);
}


/**
 * Return pseudorandom type id for T. Although highly unlikely, it's value
 * might clash with value of some unrelated type. It has advantage compared to
 * type_id() of being constexpr.
 */
template <typename T>
constexpr auto type_v = __bits::type_t<T>::id();


__sal_end
