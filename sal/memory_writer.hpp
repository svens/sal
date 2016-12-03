#pragma once

/**
 * \file sal/memory_writer.hpp
 * Memory range unformatted and formatted content writer
 */

#include <sal/config.hpp>
#include <sal/__bits/memory_writer.hpp>
#include <memory>
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


  constexpr bool is_bad () const noexcept
  {
    return begin_ > end_;
  }


  constexpr bool is_full () const noexcept
  {
    return begin_ == end_;
  }


  explicit operator bool () const noexcept
  {
    return begin_ <= end_;
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
    if (begin_ + sizeof(T) * (last - first)  <= end_)
    {
      std::uninitialized_copy(first, last,
        __bits::TODO_make_iterator(reinterpret_cast<T *>(begin_))
      );
    }
    return skip(sizeof(T) * (last - first));
  }


  template <typename T, size_t N>
  memory_writer_t &write (const T (&array)[N]) noexcept
  {
    return write(array, array + N);
  }


  template <size_t N>
  memory_writer_t &operator<< (const char (&v)[N]) noexcept
  {
    return write(v, v + N - 1);
  }


  memory_writer_t &operator<< (const char *v) noexcept
  {
    while (*v && begin_ < end_)
    {
      *begin_++ = *v++;
    }
    return *this;
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


inline memory_writer_t &operator<< (memory_writer_t &w, std::nullptr_t)
  noexcept
{
  constexpr const char label[] = "(null)";
  return w.write(label, label + sizeof(label) - 1);
}


inline memory_writer_t &operator<< (memory_writer_t &w, bool v) noexcept
{
  if (v)
  {
    constexpr const char label[] = "true";
    return w.write(label, label + sizeof(label) - 1);
  }
  constexpr const char label[] = "false";
  return w.write(label, label + sizeof(label) - 1);
}


inline memory_writer_t &operator<< (memory_writer_t &w, char v) noexcept
{
  return w.write(v);
}


inline memory_writer_t &operator<< (memory_writer_t &w, signed char v)
  noexcept
{
  return w.write(v);
}


inline memory_writer_t &operator<< (memory_writer_t &w, unsigned char v)
  noexcept
{
  return w.write(v);
}


inline memory_writer_t &operator<< (memory_writer_t &w, unsigned long long v)
  noexcept
{
  return w.skip(__bits::fmt(v, w.begin(), w.end()));
}


inline memory_writer_t &operator<< (memory_writer_t &w, long long v)
  noexcept
{
  return w.skip(__bits::fmt(v, w.begin(), w.end()));
}


inline memory_writer_t &operator<< (memory_writer_t &w, unsigned long v)
  noexcept
{
  return w.skip(
    __bits::fmt(static_cast<unsigned long long>(v), w.begin(), w.end())
  );
}


inline memory_writer_t &operator<< (memory_writer_t &w, long v)
  noexcept
{
  return w.skip(__bits::fmt(static_cast<long long>(v), w.begin(), w.end()));
}


inline memory_writer_t &operator<< (memory_writer_t &w, unsigned int v)
  noexcept
{
  return w.skip(
    __bits::fmt(static_cast<unsigned long long>(v), w.begin(), w.end())
  );
}


inline memory_writer_t &operator<< (memory_writer_t &w, int v)
  noexcept
{
  return w.skip(__bits::fmt(static_cast<long long>(v), w.begin(), w.end()));
}


inline memory_writer_t &operator<< (memory_writer_t &w, unsigned short v)
  noexcept
{
  return w.skip(
    __bits::fmt(static_cast<unsigned long long>(v), w.begin(), w.end())
  );
}


inline memory_writer_t &operator<< (memory_writer_t &w, short v)
  noexcept
{
  return w.skip(__bits::fmt(static_cast<long long>(v), w.begin(), w.end()));
}


inline memory_writer_t &operator<< (memory_writer_t &w, float v)
  noexcept
{
  return w.skip(__bits::fmt(v, "%g", w.begin(), w.end()));
}


inline memory_writer_t &operator<< (memory_writer_t &w, double v)
  noexcept
{
  return w.skip(__bits::fmt(v, "%g", w.begin(), w.end()));
}


inline memory_writer_t &operator<< (memory_writer_t &w, const long double &v)
  noexcept
{
  return w.skip(__bits::fmt(v, "%Lg", w.begin(), w.end()));
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &w, __bits::hex_t<T> v)
  noexcept
{
  return w.skip(__bits::fmt(v, w.begin(), w.end()));
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &w, __bits::oct_t<T> v)
  noexcept
{
  return w.skip(__bits::fmt(v, w.begin(), w.end()));
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &w, __bits::bin_t<T> v)
  noexcept
{
  return w.skip(__bits::fmt(v, w.begin(), w.end()));
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &w, const T *v)
  noexcept
{
  return w.skip(__bits::fmt(static_cast<const void *>(v), w.begin(), w.end()));
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &w, T *v)
  noexcept
{
  return w.skip(__bits::fmt(static_cast<const void *>(v), w.begin(), w.end()));
}


inline memory_writer_t &operator<< (memory_writer_t &w, const std::string &v)
  noexcept
{
  w.write(v.data(), v.data() + v.size());
  return w;
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &w, const T &v)
{
  std::ostringstream oss;
  if (oss << v)
  {
    w << oss.str();
  }
  return w;
}


template <typename T>
inline auto hex (T value) noexcept
{
  return __bits::hex_t<T>{value};
}


template <typename T>
inline auto oct (T value) noexcept
{
  return __bits::oct_t<T>{value};
}


template <typename T>
inline auto bin (T value) noexcept
{
  return __bits::bin_t<T>{value};
}


__sal_end
