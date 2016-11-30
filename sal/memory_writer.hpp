#pragma once

/**
 * \file sal/memory_writer.hpp
 * Locked pointer idiom implementation.
 */

#include <sal/config.hpp>
#include <memory>

#if defined(_MSC_VER)
  // see make_iterator()
  #include <iterator>
#endif


__sal_begin


template <typename T, typename R=void>
using enforce_pod = std::enable_if_t<std::is_pod<T>::value, R>;


namespace __bits {

// helper to silence MSVC 'Checked Iterators' warning
template <typename It>
constexpr inline auto make_iterator (It *it) noexcept
{
#if defined(_MSC_VER)
  return stdext::make_unchecked_array_iterator(it);
#else
  return it;
#endif
}

} // namespace __bits


class memory_writer_t
{
public:

  memory_writer_t (const memory_writer_t &) = delete;
  memory_writer_t &operator= (const memory_writer_t &) = delete;
  memory_writer_t &operator= (memory_writer_t &&w) = delete;


  template <typename T>
  constexpr memory_writer_t (T *begin, T *end) noexcept
    : begin_(reinterpret_cast<char *>(begin))
    , end_(reinterpret_cast<char *>(end))
  {
    static_assert(std::is_pod<T>::value, "expected POD type");
  }


  template <typename T, size_t N>
  constexpr memory_writer_t (T (&array)[N]) noexcept
    : memory_writer_t{array, array + N}
  {}


  memory_writer_t (memory_writer_t &&w) noexcept
    : begin_(w.begin_)
    , end_(w.end_)
  {
    w.begin_ = w.end_;
  }


  explicit operator bool () const noexcept
  {
    return begin_ < end_;
  }


  constexpr bool full () const noexcept
  {
    return begin_ >= end_;
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
  auto write (T v) noexcept
    -> enforce_pod<T>
  {
    *reinterpret_cast<T *>(begin_) = v;
    begin_ += sizeof(v);
  }


  template <typename T>
  auto try_write (T v) noexcept
    -> enforce_pod<T, bool>
  {
    if (begin_ + sizeof(v) <= end_)
    {
      write(v);
      return true;
    }
    return false;
  }


  template <typename T>
  auto write (const T *first, const T *last) noexcept
    -> enforce_pod<T>
  {
    std::uninitialized_copy(first, last,
      __bits::make_iterator(reinterpret_cast<T *>(begin_))
    );
    begin_ += sizeof(T)*(last - first);
  }


  template <typename T>
  auto try_write (const T *first, const T *last) noexcept
    -> enforce_pod<T, bool>
  {
    auto bytes = sizeof(T)*(last - first);
    if (begin_ + bytes <= end_)
    {
      return write(first, last), true;
    }
    return false;
  }


  template <typename T, size_t N>
  auto write (const T (&array)[N]) noexcept
    -> enforce_pod<T>
  {
    write(array, array + N);
  }


  template <typename T, size_t N>
  auto try_write (const T (&array)[N]) noexcept
    -> enforce_pod<T, bool>
  {
    return try_write(array, array + N);
  }


private:

  char *begin_;
  char * const end_;

};


__sal_end
