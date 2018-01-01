#pragma once

/**
 * \file sal/memory.hpp
 * Iterator and memory pointer helpers.
 */

#include <sal/config.hpp>
#include <iterator>
#include <memory>


__sal_begin


namespace __bits {

template <typename T> constexpr const bool is_const_ref_v = false;
template <typename T> constexpr const bool is_const_ref_v<const T &> = true;

template <typename It>
inline constexpr void ensure_iterator_constraints () noexcept
{
  static_assert(
    std::is_pod_v<typename std::iterator_traits<It>::value_type>,
    "expected iterator to point to POD type"
  );

  static_assert(
    std::is_base_of_v<
      std::random_access_iterator_tag,
      typename std::iterator_traits<It>::iterator_category
    >,
    "expected random access iterator"
  );
}

// internal helper: silence MSVC silly-warning C4996 (Checked Iterators)
template <typename It>
inline auto make_output_iterator (It first, It) noexcept
{
#if defined(_MSC_VER)
  return stdext::make_unchecked_array_iterator(first);
#else
  return first;
#endif
}

} // namespace __bits


/**
 * Return \a it as pointer casted to uint8_t pointer. \a it can be iterator or
 * unrelated type of pointer.
 */
template <typename It>
inline auto to_ptr (It it) noexcept
{
  __bits::ensure_iterator_constraints<It>();
  if constexpr (__bits::is_const_ref_v<decltype(*it)>)
  {
    return reinterpret_cast<const uint8_t *>(std::addressof(*it));
  }
  else
  {
    return reinterpret_cast<uint8_t *>(std::addressof(*it));
  }
}


/**
 * Return memory region [\a first, \a last) size in bytes.
 * If \a first > \a last, result is undefined.
 */
template <typename It>
inline constexpr size_t range_size (It first, It last) noexcept
{
  __bits::ensure_iterator_constraints<It>();
  return std::distance(first, last)
    * sizeof(typename std::iterator_traits<It>::value_type);
}


/**
 * Return iterator casted to uint8_t pointer to past last item in range
 * [\a first, \a last).
 */
template <typename It>
inline auto to_end_ptr (It first, It last) noexcept
{
  return to_ptr(first) + range_size(first, last);
}


__sal_end
