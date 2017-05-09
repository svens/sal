#pragma once

/**
 * \file sal/buf_ptr.hpp
 */

#include <sal/config.hpp>
#include <sal/char_array.hpp>
#include <algorithm>
#include <array>
#include <string>
#include <vector>


__sal_begin


/**
 * buf_ptr represents contiguous region of raw memory with specified size.
 * Internally it is kept as pointer pair to beginning and one byte past
 * end of region. Object of this class does not own pointed region. It is
 * application responsibility to handle lifecycle of buf_ptr and pointed
 * region.
 */
class buf_ptr
{
public:

  buf_ptr () = default;


  /**
   * Construct buf_ptr pointing to \a region with \a size.
   */
  buf_ptr (void *region, size_t size) noexcept
    : begin_(static_cast<char *>(region))
    , end_(begin_ + size)
  {}


  /**
   * Return byte pointer to beginning of region.
   */
  uint8_t *begin () const noexcept
  {
    return reinterpret_cast<uint8_t *>(begin_);
  }


  /**
   * Return byte pointer to end of region.
   */
  uint8_t *end () const noexcept
  {
    return reinterpret_cast<uint8_t *>(end_);
  }


  /**
   * Return raw pointer to beginning of region.
   */
  void *data () const noexcept
  {
    return begin_;
  }


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
  buf_ptr &operator+= (size_t s) noexcept
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
inline buf_ptr operator+ (const buf_ptr &p, size_t n) noexcept
{
  auto r = p;
  r += n;
  return r;
}


/**
 * Create and return new ptr from \a p with beginning of region moved
 * toward end by \a n bytes.
 */
inline buf_ptr operator+ (size_t n, const buf_ptr &p) noexcept
{
  auto r = p;
  r += n;
  return r;
}


/**
 * Construct buf_ptr pointing to \a region with \a size.
 */
inline buf_ptr make_buf (void *region, size_t size) noexcept
{
  return buf_ptr(region, size);
}


/**
 * Construct new buf_ptr as copy of \a ptr.
 */
inline buf_ptr make_buf (const buf_ptr &ptr) noexcept
{
  return buf_ptr(ptr.get(), ptr.size());
}


/**
 * Construct new buf_ptr as copy of \a ptr with up \a max_bytes.
 */
inline buf_ptr make_buf (const buf_ptr &ptr, size_t max_bytes) noexcept
{
  return buf_ptr(ptr.get(), (std::min)(max_bytes, ptr.size()));
}


/**
 * Construct buf_ptr using region owned by \a data
 */
template <size_t N>
inline buf_ptr make_buf (char_array_t<N> &data) noexcept
{
  // not nice, but ok: returned area is initialized
  return buf_ptr(const_cast<char *>(data.data()), data.size());
}


/**
 * Construct buf_ptr using region owned by \a data.
 */
template <typename T, size_t N>
inline buf_ptr make_buf (T (&data)[N]) noexcept
{
  return buf_ptr(data, N * sizeof(T));
}


/**
 * Construct buf_ptr using region owned by \a data.
 */
template <typename T, size_t N>
inline buf_ptr make_buf (std::array<T, N> &data) noexcept
{
  return buf_ptr(data.data(), data.size() * sizeof(T));
}


/**
 * Construct buf_ptr using region owned by \a data. Returned object is
 * invalidated by any vector operation that invalidates all references,
 * pointers and iterators referring to the elements in the sequence.
 */
template <typename T, typename Allocator>
inline buf_ptr make_buf (std::vector<T, Allocator> &data) noexcept
{
  return buf_ptr(data.data(), data.size() * sizeof(T));
}


/**
 * Construct buf_ptr using region owned by \a data. Returned object is
 * invalidated by any string operation that invalidates all references,
 * pointers and iterators referring to the elements in the sequence.
 */
template <typename T, typename Traits, typename Allocator>
inline buf_ptr make_buf (std::basic_string<T, Traits, Allocator> &data)
  noexcept
{
  return buf_ptr(&data[0], data.size() * sizeof(T));
}


/**
 * Construct buf_ptr using region owned by \a data but not more than
 * \a max_bytes
 */
template <size_t N>
inline buf_ptr make_buf (char_array_t<N> &data, size_t max_bytes) noexcept
{
  // not nice, but ok: returned area is initialized
  return buf_ptr(const_cast<char *>(data.data()),
    (std::min)(max_bytes, data.size())
  );
}


/**
 * Construct buf_ptr using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, size_t N>
inline buf_ptr make_buf (T (&data)[N], size_t max_bytes) noexcept
{
  return buf_ptr(data, (std::min)(max_bytes, N * sizeof(T)));
}


/**
 * Construct buf_ptr using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, size_t N>
inline buf_ptr make_buf (std::array<T, N> &data, size_t max_bytes) noexcept
{
  return buf_ptr(data.data(), (std::min)(max_bytes, data.size() * sizeof(T)));
}


/**
 * Construct buf_ptr using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, typename Allocator>
inline buf_ptr make_buf (std::vector<T, Allocator> &data, size_t max_bytes)
  noexcept
{
  return buf_ptr(data.data(), (std::min)(max_bytes, data.size() * sizeof(T)));
}


/**
 * Construct buf_ptr using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, typename Traits, typename Allocator>
inline buf_ptr make_buf (std::basic_string<T, Traits, Allocator> &data,
  size_t max_bytes) noexcept
{
  return buf_ptr(&data[0], (std::min)(max_bytes, data.size() * sizeof(T)));
}


/**
 * const_buf_ptr represents contiguous region of raw memory with specified
 * size. Internally it is kept as pointer pair to beginning and one byte past
 * end of region. Object of this class does not own pointed region. It is
 * application responsibility to handle lifecycle of buf_ptr and pointed
 * region.
 */
class const_buf_ptr
{
public:

  const_buf_ptr () = default;


  /**
   * Construct const_buf_ptr pointing to \a region with \a size.
   */
  const_buf_ptr (const void *region, size_t size) noexcept
    : begin_(static_cast<const char *>(region))
    , end_(begin_ + size)
  {}


  /**
   * Return byte pointer to beginning of region.
   */
  const uint8_t *begin () const noexcept
  {
    return reinterpret_cast<const uint8_t *>(begin_);
  }


  /**
   * Return byte pointer to end of region.
   */
  const uint8_t *end () const noexcept
  {
    return reinterpret_cast<const uint8_t *>(end_);
  }


  /**
   * Return pointer to beginning of region.
   */
  const void *data () const noexcept
  {
    return begin_;
  }


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
  const_buf_ptr &operator+= (size_t s) noexcept
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
inline const_buf_ptr operator+ (const const_buf_ptr &p, size_t n) noexcept
{
  auto r = p;
  r += n;
  return r;
}


/**
 * Create and return new ptr from \a p with beginning of region moved
 * toward end by \a n bytes.
 */
inline const_buf_ptr operator+ (size_t n, const const_buf_ptr &p) noexcept
{
  auto r = p;
  r += n;
  return r;
}


/**
 * Construct const_buf_ptr pointing to \a region with \a size.
 */
inline const_buf_ptr make_buf (const void *region, size_t size) noexcept
{
  return const_buf_ptr(region, size);
}


/**
 * Construct new const_buf_ptr as copy of \a ptr.
 */
inline const_buf_ptr make_buf (const const_buf_ptr &ptr) noexcept
{
  return const_buf_ptr(ptr.get(), ptr.size());
}


/**
 * Construct new const_buf_ptr as copy of \a ptr with up \a max_bytes.
 */
inline const_buf_ptr make_buf (const const_buf_ptr &ptr, size_t max_bytes)
  noexcept
{
  return const_buf_ptr(ptr.get(), (std::min)(max_bytes, ptr.size()));
}


/**
 * Construct const_buf_ptr using region owned by \a data
 */
template <size_t N>
inline const_buf_ptr make_buf (const char_array_t<N> &data) noexcept
{
  return const_buf_ptr(data.data(), data.size());
}


/**
 * Construct const_buf_ptr using region owned by \a data.
 */
template <typename T, size_t N>
inline const_buf_ptr make_buf (const T (&data)[N]) noexcept
{
  return const_buf_ptr(data, N * sizeof(T));
}


/**
 * Construct const_buf_ptr using region owned by \a data.
 */
template <typename T, size_t N>
inline const_buf_ptr make_buf (const std::array<T, N> &data) noexcept
{
  return const_buf_ptr(data.data(), data.size() * sizeof(T));
}


/**
 * Construct const_buf_ptr using region owned by \a data. Returned object is
 * invalidated by any vector operation that invalidates all references,
 * pointers and iterators referring to the elements in the sequence.
 */
template <typename T, typename Allocator>
inline const_buf_ptr make_buf (const std::vector<T, Allocator> &data) noexcept
{
  return const_buf_ptr(data.data(), data.size() * sizeof(T));
}


/**
 * Construct const_buf_ptr using region owned by \a data. Returned object is
 * invalidated by any string operation that invalidates all references,
 * pointers and iterators referring to the elements in the sequence.
 */
template <typename T, typename Traits, typename Allocator>
inline const_buf_ptr make_buf (
  const std::basic_string<T, Traits, Allocator> &data) noexcept
{
  return const_buf_ptr(&data[0], data.size() * sizeof(T));
}


/**
 * Construct const_buf_ptr using region owned by \a data but not more than
 * \a max_bytes.
 */
template <size_t N>
inline const_buf_ptr make_buf (const char_array_t<N> &data, size_t max_bytes)
  noexcept
{
  return const_buf_ptr(data.data(), (std::min)(max_bytes, data.size()));
}


/**
 * Construct const_buf_ptr using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, size_t N>
inline const_buf_ptr make_buf (const T (&data)[N], size_t max_bytes) noexcept
{
  return const_buf_ptr(data, (std::min)(max_bytes, N * sizeof(T)));
}


/**
 * Construct const_buf_ptr using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, size_t N>
inline const_buf_ptr make_buf (const std::array<T, N> &data, size_t max_bytes)
  noexcept
{
  return const_buf_ptr(data.data(),
    (std::min)(max_bytes, data.size() * sizeof(T))
  );
}


/**
 * Construct const_buf_ptr using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, typename Allocator>
inline const_buf_ptr make_buf (const std::vector<T, Allocator> &data,
  size_t max_bytes) noexcept
{
  return const_buf_ptr(data.data(),
    (std::min)(max_bytes, data.size() * sizeof(T))
  );
}


/**
 * Construct const_buf_ptr using region owned by \a data but not more than
 * \a max_bytes.
 */
template <typename T, typename Traits, typename Allocator>
inline const_buf_ptr make_buf (
  const std::basic_string<T, Traits, Allocator> &data,
  size_t max_bytes) noexcept
{
  return const_buf_ptr(&data[0],
    (std::min)(max_bytes, data.size() * sizeof(T))
  );
}


__sal_end
