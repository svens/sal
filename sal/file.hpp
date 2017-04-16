#pragma once

/**
 * \file sal/file.hpp
 * Low-level file handle wrapper
 */

#include <sal/config.hpp>
#include <sal/error.hpp>
#include <ios>
#include <string>
#include <utility>


__sal_begin


/**
 * Low-level file handle wrapper. It provides unsynchronized and unbuffered
 * file operations for all ported platforms. Depending on platform read/write
 * operations might be atomic at syscall level.
 *
 * There can be only single owner for each file (like std::unique_ptr).
 */
class file_t
{
public:

  /// Underlying platform's file handle
  using native_handle = uintptr_t;

  /// Invalid file handle
  static constexpr native_handle null = static_cast<native_handle>(-1);

  /// Alias for std::ios_base::openmode
  using open_mode = std::ios::openmode;

  /// Alias for std::ios_base::seekdir
  using seek_dir = std::ios::seekdir;


  /// Create new file with \a name. Must not exist before call.
  /// On failure \a error has reason and file with null is returned
  static file_t create (const std::string &name, open_mode mode,
    std::error_code &error
  ) noexcept;


  /// \copybrief create
  /// \throws std::system_error on error
  static file_t create (const std::string &name, open_mode mode)
  {
    return create(name, mode, throw_on_error("file_t::create"));
  }


  /// Open file with \a name. File must exist
  /// On failure \a error has reason and file with null is returned
  static file_t open (const std::string &name, open_mode mode,
    std::error_code &error
  ) noexcept;


  /// \copybrief open
  /// \throws std::system_error on error
  static file_t open (const std::string &name, open_mode mode)
  {
    return open(name, mode, throw_on_error("file_t::open"));
  }


  /// Open file with \a name. If it does not exist, it will be created.
  /// On failure \a error has reason and file with null is returned
  static file_t open_or_create (const std::string &name, open_mode mode,
    std::error_code &error
  ) noexcept;


  /// \copybrief open_or_create
  /// \throws std::system_error on error
  static file_t open_or_create (const std::string &name, open_mode mode)
  {
    return open_or_create(name, mode, throw_on_error("file_t::open_or_create"));
  }


  /// Create new file with random filename that is returned in \a name. On
  /// entry \a name must have initial string that is used as prefix for
  /// generated filename. Application should not rely on any specific format
  /// of returned \a name (even having prefix specified by original \a
  /// name). File can be created in different directory than current, if
  /// \a name has path on entry.
  static file_t unique (std::string &name, std::error_code &error) noexcept;


  /// \copydoc unique
  /// \throws std::system_error on error
  static file_t unique (std::string &name)
  {
    return unique(name, throw_on_error("file_t::unique"));
  }


  file_t () noexcept = default;


  ~file_t () noexcept
  {
    ensure_closed();
  }


  /// Create new file from \a that. Ownership of file handle in \a that
  /// moves to \a this
  file_t (file_t &&that) noexcept
    : handle_(that.handle_)
  {
    that.handle_ = null;
  }


  /// Move ownerhsip of \a that to \a this.
  file_t &operator= (file_t &&that) noexcept
  {
    ensure_closed();
    swap(*this, that);
    return *this;
  }


  /// Swap handles of \a a and \a b
  friend inline void swap (file_t &a, file_t &b) noexcept
  {
    using std::swap;
    swap(a.handle_, b.handle_);
  }


  /// Close file handle. On failure, error is set.
  void close (std::error_code &error) noexcept;


  /// \copydoc close
  /// \throws std::system_error on error
  void close ()
  {
    close(throw_on_error("file_t::close"));
  }


  /// Return true if \a this file is opened.
  bool is_open () const noexcept
  {
    return handle_ != null;
  }


  /// \copydoc is_open
  explicit operator bool () const noexcept
  {
    return is_open();
  }


  /// Attempt to write \a size bytes of \a data to file. Returns number of
  /// bytes actually written.
  size_t write (const char *data, size_t size, std::error_code &error)
    noexcept;


  /// \copydoc write
  /// \throws std::system_error on write error
  size_t write (const char *data, size_t size)
  {
    return write(data, size, throw_on_error("file_t::write"));
  }


  /// Attempt to read maximum \a size bytes into \a data. Returns number of
  /// bytes actually read.
  size_t read (char *data, size_t size, std::error_code &error)
    noexcept;


  /// \copydoc read
  /// \throws std::system_error on read error
  size_t read (char *data, size_t size)
  {
    return read(data, size, throw_on_error("file_t::read"));
  }


  /// Move file pointer by \a offset from \a whence. This method allows to
  /// move file pointer beyond the existing end of file. If data is later
  /// written at that point, following reads from gap return bytes of zeroes
  /// (until data is actually written into the gap).
  ///
  /// \returns New pointer position from beginning of file.
  int64_t seek (int64_t offset, seek_dir whence, std::error_code &error)
    noexcept;


  /// \copydoc seek
  /// \throws std::system_error on seek error.
  int64_t seek (int64_t offset, seek_dir whence)
  {
    return seek(offset, whence, throw_on_error("file_t::seek"));
  }


private:

    native_handle handle_ = null;

    file_t (native_handle handle)
        : handle_(handle)
    {}

    file_t (const file_t &) = delete;
    file_t &operator= (const file_t &) = delete;

    void ensure_closed () noexcept
    {
      if (is_open())
      {
        std::error_code error;
        close(error);
      }
    }
};


__sal_end
