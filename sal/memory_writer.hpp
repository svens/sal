#pragma once

/**
 * \file sal/memory_writer.hpp
 * Memory range unformatted and formatted content writer
 */

#include <sal/config.hpp>
#include <sal/utility.hpp>
#include <sstream>


__sal_begin


class memory_writer_t
{
public:

  memory_writer_t (const memory_writer_t &) = delete;
  memory_writer_t &operator= (const memory_writer_t &) = delete;


  template <typename T>
  constexpr memory_writer_t (T *begin, const T *end) noexcept
    : begin_{reinterpret_cast<char *>(begin)}
    , end_{reinterpret_cast<const char *>(end)}
  {
    enforce_pod<T>();
  }


  template <typename T, size_t N>
  constexpr memory_writer_t (T (&array)[N]) noexcept
    : memory_writer_t{array, array + N}
  {}


  memory_writer_t (memory_writer_t &&w) noexcept
    : begin_{w.begin_}
    , end_{w.end_}
  {
    w.begin_ = const_cast<char *>(w.end_ + 1);
  }


  memory_writer_t &operator= (memory_writer_t &&w) noexcept
  {
    begin_ = w.begin_;
    end_ = w.end_;
    w.begin_ = const_cast<char *>(w.end_ + 1);
    return *this;
  }


  void swap (memory_writer_t &that) noexcept
  {
    std::swap(begin_, that.begin_);
    std::swap(end_, that.end_);
  }


  constexpr bool good () const noexcept
  {
    return begin_ <= end_;
  }


  constexpr explicit operator bool () const noexcept
  {
    return good();
  }


  constexpr bool bad () const noexcept
  {
    return begin_ > end_;
  }


  constexpr bool full () const noexcept
  {
    return begin_ == end_;
  }


  constexpr size_t size () const noexcept
  {
    return end_ - begin_;
  }


  constexpr char *begin () const noexcept
  {
    return begin_;
  }


  constexpr const char *end () const noexcept
  {
    return end_;
  }


  memory_writer_t &skip (size_t n) noexcept
  {
    begin_ += n;
    return *this;
  }


  template <typename T>
  memory_writer_t &write (T v) noexcept
  {
    enforce_pod<T>();
    if (begin_ + sizeof(v) <= end_)
    {
      *reinterpret_cast<T *>(begin_) = v;
    }
    return skip(sizeof(v));
  }


  template <typename T>
  memory_writer_t &write (const T *first, const T *last) noexcept
  {
    enforce_pod<T>();
    begin_ = copy(begin_,
      const_cast<char *>(end_),
      reinterpret_cast<const char *>(first),
      reinterpret_cast<const char *>(last)
    );
    return *this;
  }


  template <typename T, size_t N>
  memory_writer_t &write (const T (&array)[N]) noexcept
  {
    return write(array, array + N);
  }


private:

  char *begin_;
  const char *end_;

  template <typename T>
  constexpr void enforce_pod () const noexcept
  {
    static_assert(std::is_pod<T>::value, "expected POD type");
  }
};


__sal_end
