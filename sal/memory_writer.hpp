#pragma once

/**
 * \file sal/memory_writer.hpp
 * Memory range unformatted content writer
 *
 * \see sal/format.hpp
 */

#include <sal/config.hpp>
#include <cstring>
#include <utility>


__sal_begin


/**
 * Wrapper class for memory range [pair.first, pair.second). It provides
 * methods to fill specified range with unformatted content. There are also
 * free functions (operator<<) to fill range with formatted content.
 *
 * During instance creation, memory range is specified: first is write pointer
 * where additional content is written and second is upper limit pointer.
 * While filling content, write pointer is moved towards upper limit (i.e. no
 * original beginning is remembered). When write pointer reaches upper limit,
 * no more content will be actually copied but pointer is still moved forward.
 * In such situation, object state becomes bad() and size() method return
 * value is undefined.
 */
class memory_writer_t
  : public std::pair<char *, const char *>
{
public:

  memory_writer_t (const memory_writer_t &) = delete;
  memory_writer_t &operator= (const memory_writer_t &) = delete;


  /**
   * Construct memory writer using range [begin, end)
   */
  template <typename T>
  constexpr memory_writer_t (T *begin, const T *end) noexcept
    : pair{char_p(begin), char_p(end)}
  {}


  /**
   * Construct memory writer using range [array, array + N)
   */
  template <typename T, size_t N>
  constexpr memory_writer_t (T (&array)[N]) noexcept
    : memory_writer_t{array, array + N}
  {}


  /**
   * Construct memory writer from \a that. Source object is not invalidated
   * but using it in parallel with \a this is undefined behaviour.
   */
  memory_writer_t (memory_writer_t &&that) noexcept
    : pair{std::move(that)}
  {}


  /**
   * Update \a this using range from \a that. Source object is not invalidated
   * but using it in parallel with \a this is undefined behaviour.
   */
  memory_writer_t &operator= (memory_writer_t &&that) noexcept
  {
    pair::operator=(std::move(that));
    return *this;
  }


  /**
   * Swap range of \a this with \a that.
   */
  void swap (memory_writer_t &that) noexcept
  {
    pair::swap(that);
  }


  /**
   * Return true if write pointer is less or equal to end pointer.
   * Writer pointer being equal to upper limit is considered valid state as
   * well because whole range is still valid.
   */
  constexpr bool good () const noexcept
  {
    return first <= second;
  }


  /**
   * \see good()
   */
  constexpr explicit operator bool () const noexcept
  {
    return good();
  }


  /**
   * Return true if write pointer has moved past upper limit. While object is
   * in bad() state, write() methods update write pointer but do not actually
   * add any new content.
   */
  constexpr bool bad () const noexcept
  {
    return first > second;
  }


  /**
   * Return true if write pointer has reached upper limit. If write pointer
   * has moved past upper limit is considered as bad() state instead of
   * full().
   */
  constexpr bool full () const noexcept
  {
    return first == second;
  }


  /**
   * Return number of bytes between writer pointer and upper limit. While in
   * bad() state, this method's returned value is undefined.
   */
  constexpr size_t size () const noexcept
  {
    return second - first;
  }


  /**
   * Return number of bytes between writer pointer and upper limit. In bad()
   * state it returns 0.
   */
  constexpr size_t safe_size () const noexcept
  {
    return good() ? size() : 0;
  }


  /**
   * Return current write pointer
   */
  constexpr char *begin () const noexcept
  {
    return first;
  }


  /**
   * Return upper limit pointer (one byte past actual fillable memory area)
   */
  constexpr const char *end () const noexcept
  {
    return second;
  }


  /**
   * Move write pointer towards upper limit \a n bytes without adding any new
   * content.
   */
  memory_writer_t &skip (size_t n) noexcept
  {
    first += n;
    return *this;
  }


  /**
   * Move write pointer towards upper limit until character \a ch is found.
   * If there is no \a ch before end(), search stops there.
   */
  memory_writer_t &skip_until (char ch) noexcept
  {
    while (first < second && *first != ch)
    {
      ++first;
    }
    return *this;
  }


  /**
   * Copy memory content of \a v to \a this memory writer area and move write
   * pointer towards upper limit by \c sizeof(v). If write pointer is already
   * past upper limit, no content is actually copied but pointer is still
   * moved forward.
   */
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


  /**
   * Copy memory content of continuous memory area [\a begin, \a end) to \a
   * this memory writer area and move write pointer towards upper limit by
   * number of bytes between specified range. If write pointer is already past
   * upper limit, no content is actually copied but pointer is still moved
   * forward.
   */
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


  /**
   * Write [\a array, \a array + \a N) to \a this memory area.
   * \see write(const T *, const T *)
   */
  template <typename T, size_t N>
  memory_writer_t &write (const T (&array)[N]) noexcept
  {
    return write(array, array + N);
  }


  /**
   * Copy NUL-terminated string content to this memory area. When upper limit
   * is reached, no content is copied but write pointer is still moved
   * forward.
   */
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


  /**
   * Insert single character and advance write pointer. When upper limit is
   * reached, no content is copied but write pointer is still moved forward.
   */
  memory_writer_t &operator<< (char value) noexcept
  {
    if (first < second)
    {
      *first = value;
    }
    ++first;
    return *this;
  }


  /**
   * \see operator<<(char)
   */
  memory_writer_t &operator<< (unsigned char value) noexcept
  {
    return (*this << static_cast<char>(value));
  }


  /**
   * \see operator<<(char)
   */
  memory_writer_t &operator<< (signed char value) noexcept
  {
    return (*this << static_cast<char>(value));
  }


  /**
   * For each argument, invoke operator<<() ie add formatted content to
   * \a this. When upper limit is reached, no content is copied but write
   * pointer is still moved forward.
   *
   * This class has inserter operator only for NUL-terminated const char * and
   * single characters. Header file sal/format.hpp has more specialisations
   * for different POD types and std::string. By providing inserter
   * specialisations for user defined types, those will be able print() as
   * well.
   */
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
