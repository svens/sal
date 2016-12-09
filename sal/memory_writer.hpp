#pragma once

/**
 * \file sal/memory_writer.hpp
 * Memory range unformatted and formatted content writer
 */

#include <sal/config.hpp>
#include <cstring>
#include <utility>


__sal_begin


class memory_writer_t
  : public std::pair<char *, const char *>
{
public:

  memory_writer_t (const memory_writer_t &) = delete;
  memory_writer_t &operator= (const memory_writer_t &) = delete;


  template <typename T>
  constexpr memory_writer_t (T *begin, const T *end) noexcept
    : pair{char_p(begin), char_p(end)}
  {}


  template <typename T, size_t N>
  constexpr memory_writer_t (T (&array)[N]) noexcept
    : memory_writer_t{array, array + N}
  {}


  memory_writer_t (memory_writer_t &&w) noexcept
    : pair{std::move(w)}
  {}


  memory_writer_t &operator= (memory_writer_t &&w) noexcept
  {
    pair::operator=(std::move(w));
    return *this;
  }


  void swap (memory_writer_t &that) noexcept
  {
    pair::swap(that);
  }


  constexpr bool good () const noexcept
  {
    return first <= second;
  }


  constexpr explicit operator bool () const noexcept
  {
    return good();
  }


  constexpr bool bad () const noexcept
  {
    return first > second;
  }


  constexpr bool full () const noexcept
  {
    return first == second;
  }


  constexpr size_t size () const noexcept
  {
    return second - first;
  }


  constexpr char *begin () const noexcept
  {
    return first;
  }


  constexpr const char *end () const noexcept
  {
    return second;
  }


  memory_writer_t &skip (size_t n) noexcept
  {
    first += n;
    return *this;
  }


  template <typename T>
  memory_writer_t &write (T v) noexcept
  {
    if (first + sizeof(v) <= second)
    {
      *reinterpret_cast<T *>(first) = v;
    }
    first += sizeof(v);
    return *this;
  }


  template <typename T>
  memory_writer_t &write (const T *begin, const T *end) noexcept
  {
    auto size = char_p(end) - char_p(begin);
    if (first + size <= second)
    {
      std::memcpy(first, char_p(begin), size);
    }
    first += size;
    return *this;
  }


  template <typename T, size_t N>
  memory_writer_t &write (const T (&array)[N]) noexcept
  {
    return write(array, array + N);
  }


  memory_writer_t &operator<< (const char *value) noexcept
  {
    while (*value && first < second)
    {
      *first++ = *value++;
    }
    while (*value)
    {
      ++value;
      ++first;
    }
    return *this;
  }


  memory_writer_t &operator<< (char value) noexcept
  {
    if (first < second)
    {
      *first = value;
    }
    ++first;
    return *this;
  }


  memory_writer_t &operator<< (unsigned char value) noexcept
  {
    return (*this << static_cast<char>(value));
  }


  memory_writer_t &operator<< (signed char value) noexcept
  {
    return (*this << static_cast<char>(value));
  }


  template <typename Arg, typename... Args>
  memory_writer_t &print (Arg &&arg, Args &&...args) noexcept
  {
    bool unused[] = { (*this << arg, false), (*this << args, false)... };
    (void)unused;
    return *this;
  }


private:

  template <typename T>
  constexpr char *char_p (T *p) const noexcept
  {
    static_assert(std::is_pod<T>::value, "expected POD type");
    return reinterpret_cast<char *>(p);
  }

  template <typename T>
  constexpr const char *char_p (const T *p) const noexcept
  {
    static_assert(std::is_pod<T>::value, "expected POD type");
    return reinterpret_cast<const char *>(p);
  }
};


__sal_end
