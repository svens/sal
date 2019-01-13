#pragma once

/**
 * \file sal/memory.hpp
 * Iterator and memory pointer helpers.
 */

#include <sal/config.hpp>
#include <iterator>
#include <memory>
#include <string_view>


__sal_begin


namespace __bits {

template <typename T> constexpr bool is_const_ref_v = false;
template <typename T> constexpr bool is_const_ref_v<const T &> = true;

template <typename It>
constexpr void ensure_iterator_constraints () noexcept
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
constexpr auto make_output_iterator (It first, It) noexcept
{
#if defined(_MSC_VER)
  return stdext::make_unchecked_array_iterator(first);
#else
  return first;
#endif
}

} // namespace __bits


/**
 * Return \a it as pointer casted to uint8_t pointer. \a it can be iterator
 * or unrelated type of pointer.
 */
template <typename It>
constexpr auto to_ptr (It it) noexcept
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
 * Return \a it as pointer casted to uint8_t pointer. \a it can be iterator
 * or unrelated type of pointer.
 */
constexpr std::nullptr_t to_ptr (std::nullptr_t) noexcept
{
  return nullptr;
}


/**
 * Return memory region [\a first, \a last) size in bytes.
 * If \a first > \a last, result is undefined.
 */
template <typename It>
constexpr size_t range_size (It first, It last) noexcept
{
  __bits::ensure_iterator_constraints<It>();
  return std::distance(first, last)
    * sizeof(typename std::iterator_traits<It>::value_type);
}


/**
 * Return memory region [\a first, \a last) size in bytes.
 * If \a first > \a last, result is undefined.
 */
constexpr size_t range_size (std::nullptr_t, std::nullptr_t) noexcept
{
  return 0;
}


/**
 * Return iterator casted to uint8_t pointer to past last item in range
 * [\a first, \a last).
 */
template <typename It>
constexpr auto to_end_ptr (It first, It last) noexcept
{
  return to_ptr(first) + range_size(first, last);
}


/**
 * Return iterator casted to uint8_t pointer to past last item in range
 * [\a first, \a last).
 */
constexpr std::nullptr_t to_end_ptr (std::nullptr_t, std::nullptr_t) noexcept
{
  return nullptr;
}


/**
 * Return range [\a ptr, \a ptr + length) as std::basic_string_view<T>.
 */
template <typename T>
constexpr std::basic_string_view<T> to_view (const T *ptr, size_t length)
  noexcept
{
  return {ptr, length};
}


/**
 * Return range [\a first, \a last) as std::basic_string_view<T>.
 */
template <typename It>
constexpr auto to_view (It first, It last) noexcept
  -> std::basic_string_view<typename std::iterator_traits<It>::value_type>
{
#if _MSC_VER && _DEBUG
  if (first == last)
  {
    if constexpr (std::is_pointer_v<It>)
    {
      return {first, 0U};
    }
    else
    {
      return {};
    }
  }
#endif
  return to_view(std::addressof(*first), std::distance(first, last));
}


/**
 * Return \a str as std::basic_string_view<T>.
 */
template <typename T>
constexpr std::basic_string_view<T> to_view (const T *str) noexcept
{
  return {str};
}


/**
 * Return \a container as std::basic_string_view<T>
 */
template <typename Container>
constexpr auto to_view (const Container &container) noexcept
  -> decltype(to_view(std::data(container), std::size(container)))
{
  return to_view(std::data(container), std::size(container));
}


__sal_end
