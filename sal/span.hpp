#pragma once

/**
 * \file sal/span.hpp
 *
 * Temporary partial std::span implementation until it is provided by
 * clang++, g++ and MSVC.
 */

#include <sal/config.hpp>
#include <iterator>
#include <type_traits>


__sal_begin


/**
 * \see https://en.cppreference.com/w/cpp/container/span
 */
template <typename T>
class span_t
{
public:

  /**
   * Span element type
   */
  using element_type = T;

  /**
   * Span value type
   */
  using value_type = std::remove_cv_t<T>;

  /**
   * Span index type
   */
  using index_type = std::ptrdiff_t;

  /**
   * Difference type between two span's pointer's
   */
  using difference_type = std::ptrdiff_t;

  /**
   * Span element pointer type
   */
  using pointer = T *;

  /**
   * Span element reference type
   */
  using reference = T &;

  /**
   * Span element const pointer type
   */
  using const_pointer = const T *;

  /**
   * Span element const reference type
   */
  using const_reference = const T &;

  /**
   * Span iterator type
   */
  using iterator = pointer;

  /**
   * Span const iterator type
   */
  using const_iterator = const_pointer;


  constexpr span_t () noexcept = default;


  /**
   * Assign \a that to \e this.
   */
  constexpr span_t &operator= (const span_t &that) noexcept = default;


  /**
   * Construct span over range [\a ptr, \a ptr + \a count)
   */
  constexpr span_t (pointer ptr, index_type count) noexcept
    : ptr_(ptr)
    , count_(count)
  { }


  /**
   * Construct span over range [\a first, \a last)
   */
  constexpr span_t (pointer first, pointer last) noexcept
    : span_t(first, last - first)
  { }


  /**
   * Construct span over \a arr
   */
  template <size_t N>
  constexpr span_t (element_type (&arr)[N]) noexcept
    : span_t(std::addressof(arr[0]), N)
  { }


  /**
   * Construct span over \a container
   */
  template <typename Container>
  constexpr span_t (Container &container)
    : span_t(std::data(container), std::size(container))
  { }


  /**
   * Construct const span over \a container
   */
  template <typename Container>
  constexpr span_t (const Container &container)
    : span_t(std::data(container), std::size(container))
  { }


  /**
   * Return true if size() == 0
   */
  constexpr bool empty () const noexcept
  {
    return size() == 0;
  }


  /**
   * Return pointer to beginning of span
   */
  constexpr pointer data () const noexcept
  {
    return ptr_;
  }


  /**
   * Return span element on \a index
   */
  constexpr reference operator[] (index_type index) const
  {
    return ptr_[index];
  }


  /**
   * Return number of elements in span
   */
  constexpr index_type size () const noexcept
  {
    return count_;
  }


  /**
   * Return size of span in bytes
   */
  constexpr index_type size_bytes () const noexcept
  {
    return size() * sizeof(element_type);
  }


  /**
   * Return iterator to beginning of span
   */
  constexpr iterator begin () const noexcept
  {
    return ptr_;
  }


  /**
   * Return const iterator to beginning of span
   */
  constexpr const_iterator cbegin () const noexcept
  {
    return ptr_;
  }


  /**
   * Return iterator to past end of span
   */
  constexpr iterator end () const noexcept
  {
    return ptr_ + count_;
  }


  /**
   * Return const iterator to past end of span
   */
  constexpr const_iterator cend () const noexcept
  {
    return ptr_ + count_;
  }


private:

  pointer ptr_{};
  size_t count_{};
};


/**
 * Return new const span with std::byte value type over existing \a span 
 */
template <typename T>
inline span_t<const std::byte> as_bytes (span_t<T> span) noexcept
{
  return {reinterpret_cast<const std::byte *>(span.data()), span.size_bytes()};
}


/**
 * Return new span with std::byte value type over existing \a span 
 */
template <typename T, std::enable_if_t<std::is_const_v<T> == false, int> = 0>
inline span_t<std::byte> as_writable_bytes (span_t<T> span) noexcept
{
  return {reinterpret_cast<std::byte *>(span.data()), span.size_bytes()};
}


__sal_end
