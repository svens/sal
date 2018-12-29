#pragma once

/**
 * \file sal/span.hpp
 *
 * Temporary partial std::span implementation until it is provided by
 * clang++, g++ and MSVC.
 */

#include <sal/config.hpp>
#include <array>
#include <iterator>
#include <type_traits>


__sal_begin


/**
 * \see https://en.cppreference.com/w/cpp/container/span
 */
template <typename T>
class span_t
{
private:

  template <typename Container>
  using enable_if_convertible_from = std::enable_if_t<
    std::is_convertible_v<
      std::remove_pointer_t<
        decltype(std::data(std::declval<Container &>()))
      >(*)[],
      T(*)[]
    >
    &&
    std::is_integral_v<
      std::decay_t<
        decltype(std::size(std::declval<Container &>()))
      >
    >
  >;

  template <typename U>
  using enable_if_mutable = std::enable_if_t<!std::is_const_v<T>, U>;

  template <typename U>
  using enable_if_const = std::enable_if_t<std::is_const_v<T>, U>;

public:

  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = T *;
  using reference = T &;
  using const_pointer = const T *;
  using const_reference = const T &;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;


  constexpr span_t () noexcept = default;


  /**
   * Assign \a that to \e this.
   */
  constexpr span_t &operator= (const span_t &that) noexcept = default;


  /**
   * Construct span over range [\a ptr, \a ptr + \a count)
   */
  constexpr span_t (pointer ptr, size_type count) noexcept
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
   * Construct span over \a array
   */
  template <size_t N>
  constexpr span_t (element_type (&array)[N]) noexcept
    : span_t(std::addressof(array[0]), N)
  { }


  /**
   * Construct span over \a container
   */
  template <typename Container,
    typename = enable_if_convertible_from<Container>,
    typename = enable_if_mutable<Container>
  >
  constexpr span_t (Container &container)
    : span_t(std::data(container), std::size(container))
  { }


  /**
   * Construct const span over \a container
   */
  template <typename Container,
    typename = enable_if_convertible_from<Container>,
    typename = enable_if_const<Container>
  >
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
  constexpr reference operator[] (size_type index) const
  {
    return ptr_[index];
  }


  /**
   * Return number of elements in span
   */
  constexpr size_type size () const noexcept
  {
    return count_;
  }


  /**
   * Return size of span in bytes
   */
  constexpr size_type size_bytes () const noexcept
  {
    return size() * sizeof(element_type);
  }


  /**
   * Return iterator to beginning of span
   */
  constexpr iterator begin () const noexcept
  {
    return data();
  }


  /**
   * Return const iterator to beginning of span
   */
  constexpr const_iterator cbegin () const noexcept
  {
    return begin();
  }


  /**
   * Return iterator to past end of span
   */
  constexpr iterator end () const noexcept
  {
    return data() + size();
  }


  /**
   * Return const iterator to past end of span
   */
  constexpr const_iterator cend () const noexcept
  {
    return end();
  }


  /**
   * Return iterator to beginning of span
   */
  constexpr reverse_iterator rbegin () const noexcept
  {
    return reverse_iterator(end());
  }


  /**
   * Return const iterator to beginning of span
   */
  constexpr const_reverse_iterator crbegin () const noexcept
  {
    return rbegin();
  }


  /**
   * Return iterator to past end of span
   */
  constexpr reverse_iterator rend () const noexcept
  {
    return reverse_iterator(begin());
  }


  /**
   * Return const iterator to past end of span
   */
  constexpr const_reverse_iterator crend () const noexcept
  {
    return rend();
  }


private:

  pointer ptr_{};
  size_type count_{};
};


template <typename T>
constexpr span_t<T> empty_span{};

template <typename T>
constexpr span_t<const T> empty_const_span{};


/**
 * Return span over range [\a ptr, \a ptr + \a count)
 */
template <typename T>
constexpr span_t<T> span (T *ptr, size_t count) noexcept
{
  return span_t<T>(ptr, count);
}


/**
 * Return span over range [\a first, \a last)
 */
template <typename T>
constexpr span_t<T> span (T *first, T *last) noexcept
{
  return span_t<T>(first, last);
}


/**
 * Return span over \a array
 */
template <typename T, size_t N>
constexpr span_t<T> span (T (&array)[N]) noexcept
{
  return span_t<T>(array, N);
}


/**
 * Return span over \a container
 */
template <typename Container>
constexpr auto span (Container &container) noexcept
  -> decltype(span(std::data(container), std::size(container)))
{
  return span(std::data(container), std::size(container));
}


/**
 * Return span over range [\a ptr, \a ptr + \a count)
 */
template <typename T>
constexpr span_t<const T> const_span (T *ptr, size_t count) noexcept
{
  return span_t<const T>(ptr, count);
}


/**
 * Return span over range [\a first, \a last)
 */
template <typename T>
constexpr span_t<const T> const_span (T *first, T *last) noexcept
{
  return span_t<const T>(first, last);
}


/**
 * Return span over \a array
 */
template <typename T, size_t N>
constexpr span_t<const T> const_span (const T (&array)[N]) noexcept
{
  return span_t<const T>(array, N);
}


/**
 * Return span over \a container
 */
template <typename Container>
constexpr auto const_span (const Container &container) noexcept
  -> decltype(span(container))
{
  return span(container);
}


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
template <typename T, typename = std::enable_if_t<!std::is_const_v<T>>>
inline span_t<std::byte> as_writable_bytes (span_t<T> span) noexcept
{
  return {reinterpret_cast<std::byte *>(span.data()), span.size_bytes()};
}


__sal_end
