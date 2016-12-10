#pragma once

/**
 * \file sal/char_array.hpp
 * Fixed size char[] to hold formatted text
 */


#include <sal/config.hpp>
#include <sal/memory_writer.hpp>
#include <sal/format.hpp>


__sal_begin


template <size_t Size>
class char_array_t
{
public:

  static_assert(Size > 0, "zero-sized char_array_t not allowed");


  char_array_t () noexcept
  {
    data_[Size] = '\0';
  }


  constexpr bool good () const noexcept
  {
    return writer_.good();
  }


  constexpr explicit operator bool () const noexcept
  {
    return good();
  }


  constexpr bool bad () const noexcept
  {
    return writer_.bad();
  }


  constexpr bool full () const noexcept
  {
    return writer_.full();
  }


  constexpr bool empty () const noexcept
  {
    return writer_.first == data_;
  }


  constexpr size_t size () const noexcept
  {
    return writer_.first - data_;
  }


  constexpr size_t max_size () const noexcept
  {
    return Size;
  }


  constexpr size_t available () const noexcept
  {
    return writer_.size();
  }


  const char *c_str () noexcept
  {
    *writer_.first = '\0';
    return data_;
  }


  const char *data () const noexcept
  {
    return data_;
  }


  constexpr const char *begin () const noexcept
  {
    return data_;
  }


  constexpr const char *end () const noexcept
  {
    return writer_.first;
  }


  constexpr const char &operator[] (size_t pos) const noexcept
  {
    return data_[pos];
  }


  constexpr const char &front () const noexcept
  {
    return data_[0];
  }


  constexpr const char &back () const noexcept
  {
    return writer_.first[-1];
  }


  constexpr uintptr_t mark () const noexcept
  {
    return reinterpret_cast<uintptr_t>(writer_.first);
  }


  void revert (uintptr_t marker) noexcept
  {
    writer_.first = reinterpret_cast<char *>(marker);
  }


  void reset () noexcept
  {
    writer_.first = data_;
  }


  template <typename Arg>
  char_array_t &operator<< (const Arg &value) noexcept
  {
    writer_ << value;
    return *this;
  }


  template <typename Arg, typename... Args>
  char_array_t &print (Arg &&first, Args &&...rest) noexcept
  {
    writer_.print(first, rest...);
    return *this;
  }


  std::string to_string () const
  {
    return std::string{begin(), end()};
  }


  friend memory_writer_t &operator<< (memory_writer_t &writer,
    const char_array_t &chars) noexcept
  {
    return writer.write(chars.begin(), chars.end());
  }


private:

  char data_[Size + 1];
  memory_writer_t writer_{data_, data_ + Size};
};


__sal_end
