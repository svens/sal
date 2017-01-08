#pragma once

/**
 * \file sal/ptr.hpp
 */

#include <sal/config.hpp>
#include <algorithm>
#include <array>
#include <string>
#include <vector>


__sal_begin


/**
 * ptr_t represents contiguous region of raw memory with specified size.
 * Internally it is kept as pointer pair to beginning and one byte past
 * end of region. Object of this class does not own pointed region. It is
 * application responsibility to handle lifecycle of ptr_t and pointed
 * region.
 */
class ptr_t
{
public:

  ptr_t () = default;


  /**
   * Construct ptr_t pointing to \a region with \a size.
   */
  ptr_t (void *region, size_t size) noexcept
    : begin_(static_cast<char *>(region))
    , end_(begin_ + size)
  {}


  /**
   * Return raw pointer to beginning of region.
   */
  void *get () const noexcept
  {
    return begin_;
  }


  /**
   * Return size of region
   */
  size_t size () const noexcept
  {
    return end_ - begin_;
  }


  /**
   * Move beginning of pointed region by \a s bytes toward end of region.
   */
  ptr_t &operator+= (size_t s) noexcept
  {
    begin_ += s;
    if (begin_ + s > end_)
    {
      begin_ = end_;
    }
    return *this;
  }


private:

  char *begin_{}, * const end_{};
};


/**
 * Create and return new ptr from \a p with beginning of region moved
 * toward end by \a n bytes.
 */
inline ptr_t operator+ (const ptr_t &p, size_t n) noexcept
{
  auto r = p;
  r += n;
  return r;
}


/**
 * Create and return new ptr from \a p with beginning of region moved
 * toward end by \a n bytes.
 */
inline ptr_t operator+ (size_t n, const ptr_t &p) noexcept
{
  auto r = p;
  r += n;
  return r;
}


/**
 * Construct ptr_t pointing to \a region with \a size.
 */
inline ptr_t ptr (void *region, size_t size) noexcept
{
  return ptr_t(region, size);
}


/**
 * Construct new ptr_t as copy of \a ptr.
 */
inline ptr_t ptr (const ptr_t &ptr) noexcept
{
  return ptr_t(ptr.get(), ptr.size());
}


/**
 * Construct new ptr_t as copy of \a ptr with up \a max_bytes.
 */
inline ptr_t ptr (const ptr_t &ptr, size_t max_bytes) noexcept
{
  return ptr_t(ptr.get(), (std::min)(max_bytes, ptr.size()));
}


/**
 * Construct ptr_t using region owned by \a data.
 */
template <typename T, size_t N>
inline ptr_t ptr (T (&data)[N]) noexcept
{
  return ptr_t(data, N * sizeof(T));
}


/**
 * Construct ptr_t using region owned by \a data.
 */
template <typename T, size_t N>
inline ptr_t ptr (std::array<T, N> &data) noexcept
{
  return ptr_t(data.data(), data.size() * sizeof(T));
}


/**
 * Construct ptr_t using region owned by \a data. Returned object is
 * invalidated by any vector operation that invalidates all references,
 * pointers and iterators referring to the elements in the sequence.
 */
template <typename T, typename Allocator>
inline ptr_t ptr (std::vector<T, Allocator> &data) noexcept
{
  return ptr_t(data.data(), data.size() * sizeof(T));
}


/**
 * Construct ptr_t using region owned by \a data. Returned object is
 * invalidated by any string operation that invalidates all references,
 * pointers and iterators referring to the elements in the sequence.
 */
template <typename T, typename Traits, typename Allocator>
inline ptr_t ptr (std::basic_string<T, Traits, Allocator> &data)
  noexcept
{
  return ptr_t(&data[0], data.size() * sizeof(T));
}


/**
 * Construct ptr_t using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, size_t N>
inline ptr_t ptr (T (&data)[N], size_t max_bytes) noexcept
{
  return ptr_t(data, (std::min)(max_bytes, N * sizeof(T)));
}


/**
 * Construct ptr_t using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, size_t N>
inline ptr_t ptr (std::array<T, N> &data, size_t max_bytes) noexcept
{
  return ptr_t(data.data(), (std::min)(max_bytes, data.size() * sizeof(T)));
}


/**
 * Construct ptr_t using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, typename Allocator>
inline ptr_t ptr (std::vector<T, Allocator> &data, size_t max_bytes)
  noexcept
{
  return ptr_t(data.data(), (std::min)(max_bytes, data.size() * sizeof(T)));
}


/**
 * Construct ptr_t using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, typename Traits, typename Allocator>
inline ptr_t ptr (std::basic_string<T, Traits, Allocator> &data,
  size_t max_bytes) noexcept
{
  return ptr_t(&data[0], (std::min)(max_bytes, data.size() * sizeof(T)));
}


/**
 * const_ptr_t represents contiguous region of raw memory with specified size.
 * Internally it is kept as pointer pair to beginning and one byte past end of
 * region. Object of this class does not own pointed region. It is application
 * responsibility to handle lifecycle of ptr_t and pointed region.
 */
class const_ptr_t
{
public:

  const_ptr_t () = default;


  /**
   * Construct const_ptr_t pointing to \a region with \a size.
   */
  const_ptr_t (const void *region, size_t size) noexcept
    : begin_(static_cast<const char *>(region))
    , end_(begin_ + size)
  {}


  /**
   * Return pointer to beginning of region.
   */
  const void *get () const noexcept
  {
    return begin_;
  }


  /**
   * Return size of region
   */
  size_t size () const noexcept
  {
    return end_ - begin_;
  }


  /**
   * Move beginning of pointed region by \a s bytes toward end of region.
   */
  const_ptr_t &operator+= (size_t s) noexcept
  {
    begin_ += s;
    if (begin_ > end_)
    {
      begin_ = end_;
    }
    return *this;
  }


private:

  const char *begin_{}, * const end_{};
};


/**
 * Create and return new ptr from \a p with beginning of region moved
 * toward end by \a n bytes.
 */
inline const_ptr_t operator+ (const const_ptr_t &p, size_t n) noexcept
{
  auto r = p;
  r += n;
  return r;
}


/**
 * Create and return new ptr from \a p with beginning of region moved
 * toward end by \a n bytes.
 */
inline const_ptr_t operator+ (size_t n, const const_ptr_t &p) noexcept
{
  auto r = p;
  r += n;
  return r;
}


/**
 * Construct const_ptr_t pointing to \a region with \a size.
 */
inline const_ptr_t ptr (const void *region, size_t size) noexcept
{
  return const_ptr_t(region, size);
}


/**
 * Construct new const_ptr_t as copy of \a ptr.
 */
inline const_ptr_t ptr (const const_ptr_t &ptr) noexcept
{
  return const_ptr_t(ptr.get(), ptr.size());
}


/**
 * Construct new const_ptr_t as copy of \a ptr with up \a max_bytes.
 */
inline const_ptr_t ptr (const const_ptr_t &ptr, size_t max_bytes) noexcept
{
  return const_ptr_t(ptr.get(), (std::min)(max_bytes, ptr.size()));
}


/**
 * Construct const_ptr_t using region owned by \a data.
 */
template <typename T, size_t N>
inline const_ptr_t ptr (const T (&data)[N]) noexcept
{
  return const_ptr_t(data, N * sizeof(T));
}


/**
 * Construct const_ptr_t using region owned by \a data.
 */
template <typename T, size_t N>
inline const_ptr_t ptr (const std::array<T, N> &data) noexcept
{
  return const_ptr_t(data.data(), data.size() * sizeof(T));
}


/**
 * Construct const_ptr_t using region owned by \a data. Returned object is
 * invalidated by any vector operation that invalidates all references,
 * pointers and iterators referring to the elements in the sequence.
 */
template <typename T, typename Allocator>
inline const_ptr_t ptr (const std::vector<T, Allocator> &data) noexcept
{
  return const_ptr_t(data.data(), data.size() * sizeof(T));
}


/**
 * Construct const_ptr_t using region owned by \a data. Returned object is
 * invalidated by any string operation that invalidates all references,
 * pointers and iterators referring to the elements in the sequence.
 */
template <typename T, typename Traits, typename Allocator>
inline const_ptr_t ptr (const std::basic_string<T, Traits, Allocator> &data)
  noexcept
{
  return const_ptr_t(&data[0], data.size() * sizeof(T));
}


/**
 * Construct const_ptr_t using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, size_t N>
inline const_ptr_t ptr (const T (&data)[N], size_t max_bytes) noexcept
{
  return const_ptr_t(data, (std::min)(max_bytes, N * sizeof(T)));
}


/**
 * Construct const_ptr_t using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, size_t N>
inline const_ptr_t ptr (const std::array<T, N> &data, size_t max_bytes) noexcept
{
  return const_ptr_t(data.data(), (std::min)(max_bytes, data.size() * sizeof(T)));
}


/**
 * Construct const_ptr_t using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, typename Allocator>
inline const_ptr_t ptr (const std::vector<T, Allocator> &data, size_t max_bytes)
  noexcept
{
  return const_ptr_t(data.data(), (std::min)(max_bytes, data.size() * sizeof(T)));
}


/**
 * Construct const_ptr_t using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, typename Traits, typename Allocator>
inline const_ptr_t ptr (const std::basic_string<T, Traits, Allocator> &data,
  size_t max_bytes) noexcept
{
  return const_ptr_t(&data[0], (std::min)(max_bytes, data.size() * sizeof(T)));
}


__sal_end
