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

  template <size_t N>
  static constexpr uint64_t fnv_1a_32 (const char (&buf)[N]) noexcept
  {
    uint64_t h = 0x811c9dc5;
    for (auto x = buf;  x != buf + N - 1;  ++x)
    {
      h ^= *x;
      h += static_cast<uint32_t>((h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24));
    }
    return h;
  }
};


} // namespace __bits


/**
 * Return unique id for T amongst all types.
 */
template <typename T>
inline uintptr_t type_id () noexcept
{
  return reinterpret_cast<uintptr_t>(&__bits::unique_address_for<T>);
}


/**
 * \copydoc type_id()
 */
template <typename T>
inline uintptr_t type_id (T) noexcept
{
  return reinterpret_cast<uintptr_t>(&__bits::unique_address_for<T>);
}


/**
 * Return pseudorandom type id for T. Although highly unlikely, it's value
 * might clash with value of some unrelated type. It has advantage compared to
 * type_id() of being constexpr.
 */
template <typename T>
constexpr const auto type_v = __bits::type_t<T>::id();


__sal_end
