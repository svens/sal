#pragma once

/**
 * \file sal/char_array.hpp
 * Fixed size char[] to hold formatted text
 */


#include <sal/config.hpp>
#include <sal/memory_writer.hpp>
#include <sal/format.hpp>


__sal_begin


/**
 * Wrapper class for \c char[] array to gather formatted inserted values.
 *
 * Internally it holds \c char[] with \a Size plus one byte for NUL-terminator.
 * There is also pointer to end() of current content. It is updated every time
 * new content is added. Once application is done with current content, array
 * can be reset(), so new values can be inserted again. Internal buffer has no
 * NUL-terminator until c_str() is called.
 *
 * When array overflows, pointer to end() still keeps increasing if more
 * content is inserted but no actual data is added. Existing content is still
 * valid but not iterator returned by end(). Even if end() points past
 * internal array, distance between [begin(), end()) shows how many characters
 * would have been added with operator<<() if there would been enough room.
 *
 * This class is similar to C++17 std::string_view except it owns array and
 * allows to insert content there. Same time it is missing most of
 * std::string_view find/comparison/etc functionality. Once it is standardized
 * and included by g++/clang++/msvc, new getter will be added that returns
 * non-mutable std::string_view with internal array.
 */
template <size_t Size>
class char_array_t
{
public:

  static_assert(Size > 0, "zero-sized char_array_t not allowed");


  /// Construct new empty char_array_t
  char_array_t () noexcept = default;


  /**
   * Construct new char_array_t with content from \a that. If \a that is
   * bad(), only safe_size() characters are copied.
   */
  char_array_t (const char_array_t &that) noexcept
  {
    assign(that.data(), that.safe_size());
  }


  /**
   * Construct new char_array_t with content from \a that. If \a that is
   * bad(), only safe_size() characters are copied.
   */
  template <size_t ThatSize>
  char_array_t (const char_array_t<ThatSize> &that) noexcept
  {
    assign(that.data(), that.safe_size());
  }


  /**
   * Assign content from \a that. If \a that is bad(), only safe_size()
   * characters are copied.
   */
  char_array_t &operator= (const char_array_t &that) noexcept
  {
    assign(that.data(), that.safe_size());
    return *this;
  }


  /// \copydoc operator=(const char_array_t &)
  template <size_t ThatSize>
  char_array_t &operator= (const char_array_t<ThatSize> &that) noexcept
  {
    assign(that.data(), that.safe_size());
    return *this;
  }


  /**
   * Return true if pointer to end of currently added content is valid.
   * \see memory_writer_t::good()
   */
  constexpr bool good () const noexcept
  {
    return writer_.good();
  }


  /// \copydoc good()
  constexpr explicit operator bool () const noexcept
  {
    return good();
  }


  /**
   * Return true if pointer to end of currently added content is not valid.
   * \see memory_writer_t::bad()
   */
  constexpr bool bad () const noexcept
  {
    return writer_.bad();
  }


  /**
   * Return true if internal buffer is full.
   * \see memory_writer_t::full()
   */
  constexpr bool full () const noexcept
  {
    return writer_.full();
  }


  /**
   * Return true if there is no content added to internal buffer.
   */
  constexpr bool empty () const noexcept
  {
    return writer_.first == data_;
  }


  /**
   * Return number of bytes currently in internal buffer. Returned value is
   * valid only if object is good()
   */
  constexpr size_t size () const noexcept
  {
    return writer_.first - data_;
  }


  /**
   * Return number of bytes internal buffer can hold.
   */
  constexpr size_t max_size () const noexcept
  {
    return Size;
  }


  /**
   * Return number of bytes currently in internal buffer. If object is bad(),
   * then max_size() is returned
   */
  constexpr size_t safe_size () const noexcept
  {
    return good() ? size() : max_size();
  }


  /**
   * Return available number of bytes internal buffer can hold more. Returned
   * value is valid only if object is good()
   */
  constexpr size_t available () const noexcept
  {
    return writer_.size();
  }


  /**
   * Return pointer to NUL-terminated internal buffer. Calling this method
   * while object is bad() is undefined behaviour.
   */
  const char *c_str () noexcept
  {
    *writer_.first = '\0';
    return data_;
  }


  /**
   * Return pointer to beginning of character array.
   */
  const char *data () const noexcept
  {
    return data_;
  }


  /**
   * Return pointer to beginning of character array.
   */
  constexpr const char *begin () const noexcept
  {
    return data_;
  }


  /**
   * Return pointer to end of content in character array. Returned pointer is
   * valid only if object is good().
   */
  constexpr const char *end () const noexcept
  {
    return writer_.first;
  }


  /**
   * Return reference to character at \a pos. Array bounds are not checked.
   * Attemp to access outside [begin(), end()) is undefined behaviour.
   */
  constexpr const char &operator[] (size_t pos) const noexcept
  {
    return data_[pos];
  }


  /**
   * Return reference to first character
   */
  constexpr const char &front () const noexcept
  {
    return data_[0];
  }


  /**
   * Return reference to last added character. Valid only object is good() and
   * it is not empty().
   */
  constexpr const char &back () const noexcept
  {
    return writer_.first[-1];
  }


  /**
   * Move the current end pointer backwards \a n characters (but not before
   * begin()). Calling this method while object is bad() may return object
   * to good() state if updated pointer points to internal array.
   */
  void remove_suffix (size_t n) noexcept
  {
    writer_.first -= n;
    if (writer_.first < data_)
    {
      writer_.first = data_;
    }
  }


  /**
   * Return opaque marker indicating current end of content. Application can
   * revert() later end pointer to same position. Returned value is valid only
   * if object is good()
   */
  constexpr uintptr_t mark () const noexcept
  {
    return writer_.first - data_;
  }


  /**
   * Update current end of content to \a marker (returned previously by
   * mark()). Moving pointer forward is undefined behaviour.
   */
  void revert (uintptr_t marker) noexcept
  {
    writer_.first = data_ + marker;
  }


  /**
   * Move end of content pointer to beginning of internal array ie clear
   * content.
   */
  void reset () noexcept
  {
    writer_.first = data_;
  }


  /**
   * Write human readable formatted \a value to internal buffer. This method
   * uses memory_writer_t inserter methods to add content. If new content does
   * not fit into internal buffer, nothing is written but end pointer is still
   * moved forward and object state is set to bad(). Same applies when object
   * is already in bad() state.
   */
  template <typename Arg>
  char_array_t &operator<< (const Arg &value) noexcept
  {
    writer_ << value;
    return *this;
  }


  /**
   * Print list of arguments into internal buffer.
   * \see operator<<(const Arg &)
   */
  template <typename Arg, typename... Args>
  char_array_t &print (Arg &&first, Args &&...rest) noexcept
  {
    writer_.print(first, rest...);
    return *this;
  }


  /**
   * Create and return string with content from internal buffer.
   * This call is valid only if object is good().
   */
  std::string to_string () const
  {
    return std::string{begin(), end()};
  }


  /**
   * Write currently added content to \a writer. This call is valid only if
   * \a chars is good().
   */
  friend memory_writer_t &operator<< (memory_writer_t &writer,
    const char_array_t &chars) noexcept
  {
    return writer.write(chars.begin(), chars.end());
  }


private:

  char data_[Size + 1];
  memory_writer_t writer_{data_, data_ + Size};

  void assign (const char *ptr, size_t length) noexcept
  {
    if (length > Size)
    {
      length = Size;
    }
    std::memcpy(data_, ptr, length);
    writer_.first = data_ + length;
  }
};


__sal_end
