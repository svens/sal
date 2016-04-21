#pragma once

/**
 * \file sal/view.hpp
 * Fixed size char[] to hold formatted text
 */


#include <sal/config.hpp>
#include <sal/__bits/fmtval.hpp>
#include <ostream>


namespace sal {
__sal_begin


// fwd
template <size_t Size> class view;
namespace __bits {
template <size_t Size>
char *fmt_v (const view<Size> &value, char *first, char *last) noexcept;
} // namespace __bits


/**
 * Wrapper class for char[] array to gather formatted inserted values.
 *
 * Internally it holds char[] with \a Size plus one byte for NUL-terminator.
 * There is also pointer to end() of current content. It is updated every time
 * new content is added. Once application is done with current content, array
 * can be reset(), so new values can be inserted again. This internal buffer
 * is kept always NUL-terminated.
 *
 * When array overflows, pointer to end() still keeps increasing if more
 * content is inserted but no actual data is added. Existing content is still
 * valid but not iterator returned by end(). Even if end() points past
 * internal array, distance between [begin(), end()) shows how many characters
 * would have been added with operator<<() if there would been enough room.
 *
 * Call good() to check if it is valid. To restore end pointer to valid state
 * (first position of NUL-terminator), call restore() that preserves existing
 * content. To completely reset object, call reset().
 *
 * This class is similar to C++17 std::string_view except it owns array and
 * allows to insert content there. Same time it is missing most of
 * std::string_view find/comparison/etc functionality. Once it is standardized
 * and included by g++/clang++/msvc, new getter will be added that returns
 * non-mutable std::string_view with internal array.
 */
template <size_t Size>
class view
{
public:

  static_assert(Size > 0, "zero-sized view not allowed");


  /// Construct new empty view
  view () noexcept
  {
    reset();
  }


  /// Construct new view with content from \a that
  view (const view &that) noexcept
  {
    operator=(that);
  }


  /// \copydoc view(const view &)
  template <size_t ThatSize>
  view (const view<ThatSize> &that) noexcept
  {
    operator=(that);
  }


  /// Assign new content to \a this from \a that
  view &operator= (const view &that) noexcept
  {
    end_ = fmt_v(that, begin_);
    *(good() ? end_ : begin_) = '\0';
    return *this;
  }


  /// \copydoc operator=(const view &that)
  template <size_t ThatSize>
  view &operator= (const view<ThatSize> &that) noexcept
  {
    static_assert(Size >= ThatSize, "this is smaller than that");
    end_ = fmt_v(that, begin_);
    *(good() ? end_ : begin_) = '\0';
    return *this;
  }


  //
  // iterators
  //


  /// Return pointer to beginning of array
  constexpr const char *begin () const noexcept
  {
    return begin_;
  }


  /// \copydoc begin()
  constexpr const char *cbegin () const noexcept
  {
    return begin_;
  }


  /**
   * Return pointer to end of current content in array.
   *
   * It is valid only if this is good(). If valid, it points to one byte past
   * current content (i.e. to position of NUL-terminator). If not valid,
   * distance between [begin(), end()) tells how many bytes would have been
   * inserted if there would been enough room in array.
   *
   * \see restore()
   */
  const char *end () const noexcept
  {
    return end_;
  }


  /// \copydoc end()
  const char *cend () const noexcept
  {
    return end_;
  }


  //
  // element access
  //


  /// \copydoc begin()
  constexpr const char *data () const noexcept
  {
    return begin_;
  }


  /**
   * Return reference to character at \a pos. Array bounds are not checked.
   * Attempt to access outside [begin(), end()) is undefined behaviour.
   */
  constexpr const char &operator[] (size_t pos) const noexcept
  {
    return begin_[pos];
  }


  /// Return reference to first character.
  constexpr const char &front () const noexcept
  {
    return begin_[0];
  }


  /// Return reference to last character. Valid only if good()
  constexpr const char &back () const noexcept
  {
    return end_[-1];
  }


  //
  // capacity
  //


  /**
   * Return distance between [begin(), end()).
   * \see end() about validity
   */
  size_t size () const noexcept
  {
    return end_ - begin_;
  }


  /**
   * Return size of internal buffer (not including additional room for
   * NUL-terminator)
   */
  static constexpr size_t max_size () noexcept
  {
    return Size;
  }


  /// Return true if begin() == end() i.e. there is no content in array.
  bool empty () const noexcept
  {
    return end_ == begin_;
  }


  /**
   * Return true if pointer to end of currently added content is valid i.e.
   * end() in [begin(), begin() + max_size()]
   * \see end() about validity
   */
  bool good () const noexcept
  {
    return end_ <= begin_ + Size;
  }


  //
  // modifiers
  //


  /// Wipe content of internal array and set pointers to initial state
  void reset () noexcept
  {
    *(end_ = begin_) = '\0';
  }


  /**
   * Set end() to first NUL-terminator (or begin() if there is no existing
   * content in array). Because this call points end() to valid range, it will
   * restore good() state for this object.
   */
  void restore () noexcept
  {
    for (end_ = begin_;  *end_;  ++end_) /**/;
  }


  //
  // operations
  //


  /**
   * Return pointer to C-string (NUL-terminated). As every inserter adds
   * terminator, this call returns always pointer to valid C-string, even if
   * \a this state is not good().
   */
  constexpr const char *c_str () const noexcept
  {
    return begin_;
  }


  /**
   * Create std::string from existing content in array. This call is valid
   * only if \a this state is good().
   */
  std::string str () const
  {
    return std::string(cbegin(), cend());
  }


  //
  // non-member functions
  //


  /**
   * Insert textual representation of \a value to view \a v. If \a v is not
   * good(), end() pointer is still increased but no content is actually
   * added.
   */
  template <typename T>
  friend view &operator<< (view &v, const T &value) noexcept
  {
    v.end_ = __bits::fmt_v(value, v.end_, v.begin_ + Size);
    if (v.good())
    {
      *v.end_ = '\0';
    }
    return v;
  }


private:

  char begin_[Size + 1], *end_ = begin_;
};


namespace __bits {

// specialization for fmt_v(view)
template <size_t Size>
inline char *fmt_v (const view<Size> &value, char *first, char *last) noexcept
{
  return __bits::copy_s(value.begin(), value.end(), first, last);
}

} // namespace __bits


/// Insert content of \a v to \a os. This call is valid only if \a v.good()
template <size_t Size>
std::ostream &operator<< (std::ostream &os, const view<Size> &v)
{
  return os.write(v.begin(), v.size());
}


__sal_end
} // namespace sal
