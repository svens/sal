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


template <typename T>
class span_t
{
public:

  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using index_type = std::ptrdiff_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T *;
  using reference = T &;
  using const_pointer = const T *;
  using const_reference = const T &;
  using iterator = pointer;
  using const_iterator = const_pointer;


  constexpr span_t () noexcept = default;
  constexpr span_t &operator= (const span_t &that) noexcept = default;


  constexpr span_t (pointer ptr, index_type count) noexcept
    : ptr_(ptr)
    , count_(count)
  { }


  constexpr span_t (pointer first, pointer last) noexcept
    : span_t(first, last - first)
  { }


  template <size_t N>
  constexpr span_t (element_type (&arr)[N]) noexcept
    : span_t(std::addressof(arr[0]), N)
  { }


  template <typename Container>
  constexpr span_t (Container &container)
    : span_t(std::data(container), std::size(container))
  { }


  template <typename Container>
  constexpr span_t (const Container &container)
    : span_t(std::data(container), std::size(container))
  { }


  constexpr bool empty () const noexcept
  {
    return size() == 0;
  }


  constexpr pointer data () const noexcept
  {
    return ptr_;
  }


  constexpr reference operator[] (index_type index) const
  {
    return ptr_[index];
  }


  constexpr index_type size () const noexcept
  {
    return count_;
  }


  constexpr index_type size_bytes () const noexcept
  {
    return size() * sizeof(element_type);
  }


  constexpr iterator begin () const noexcept
  {
    return ptr_;
  }


  constexpr const_iterator cbegin () const noexcept
  {
    return ptr_;
  }


  constexpr iterator end () const noexcept
  {
    return ptr_ + count_;
  }


  constexpr const_iterator cend () const noexcept
  {
    return ptr_ + count_;
  }


private:

  pointer ptr_{};
  size_t count_{};
};


template <typename T>
inline span_t<const std::byte> as_bytes (span_t<T> span) noexcept
{
  return {reinterpret_cast<const std::byte *>(span.data()), span.size_bytes()};
}


template <typename T, std::enable_if_t<std::is_const_v<T> == false, int> = 0>
inline span_t<std::byte> as_writable_bytes (span_t<T> span) noexcept
{
  return {reinterpret_cast<std::byte *>(span.data()), span.size_bytes()};
}


__sal_end
