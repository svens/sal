#include <sal/file.hpp>

#if __sal_os_windows
  #include <windows.h>
#else
  #include <fcntl.h>
  #include <sys/stat.h>
  #include <unistd.h>
#endif


__sal_begin


namespace {

constexpr inline bool is_set (file_t::open_mode mode, file_t::open_mode mask)
  noexcept
{
  return (mode & mask) == mask;
}


#if __sal_os_windows

void to_lib_mode (file_t::open_mode mode, int *access_mode, int *share_mode)
  noexcept
{
  // std::ios::trunc is handled in caller

  *access_mode = *share_mode = 0;

  if (is_set(mode, std::ios::in))
  {
    *access_mode |= GENERIC_READ;
    *share_mode |= FILE_SHARE_READ;
  }

  if (is_set(mode, std::ios::out))
  {
    *access_mode |= GENERIC_WRITE;
    *share_mode |= FILE_SHARE_WRITE;
  }

  if (is_set(mode, std::ios::app))
  {
    *access_mode &= ~GENERIC_WRITE;
    *access_mode |= FILE_APPEND_DATA;
  }
}


file_t::native_handle open_impl (const std::string &name,
  file_t::open_mode open_mode,
  int create_disposition,
  std::error_code &error) noexcept
{
  int access_mode, share_mode;
  to_lib_mode(open_mode, &access_mode, &share_mode);

  auto handle = reinterpret_cast<file_t::native_handle>(
    ::CreateFile(name.c_str(),
      access_mode,
      share_mode,
      nullptr,
      create_disposition,
      FILE_ATTRIBUTE_NORMAL,
      nullptr
    )
  );
  if (handle == file_t::null)
  {
    error.assign(::GetLastError(), std::system_category());
  }
  return handle;
}

#else

constexpr int to_lib_mode (file_t::open_mode mode) noexcept
{
  int rv = 0;

  if (is_set(mode, std::ios::in | std::ios::out))
  {
    rv |= O_RDWR;
  }
  else if (is_set(mode, std::ios::in))
  {
    rv |= O_RDONLY;
  }
  else if (is_set(mode, std::ios::out))
  {
    rv |= O_WRONLY;
  }

  if (is_set(mode, std::ios::app))
  {
    rv |= O_APPEND;
  }

  if (is_set(mode, std::ios::trunc))
  {
    rv |= O_TRUNC;
  }

  return rv;
}


inline file_t::native_handle open_impl (const std::string &name,
  int open_mode,
  std::error_code &error) noexcept
{
  file_t::native_handle handle = ::open(name.c_str(), open_mode, 0600);
  if (handle == file_t::null)
  {
    error.assign(errno, std::generic_category());
  }
  return handle;
}

#endif

} // namespace


file_t file_t::create (const std::string &name, open_mode mode,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  return open_impl(name, mode, CREATE_NEW, error);

#else

  auto open_mode = to_lib_mode(mode) | O_CREAT | O_EXCL;
  return open_impl(name, open_mode, error);

#endif
}


file_t file_t::open (const std::string &name, open_mode mode,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  int create_disposition = is_set(mode, std::ios::trunc)
    ? TRUNCATE_EXISTING
    : OPEN_EXISTING;
  return open_impl(name, mode, create_disposition, error);

#else

  return open_impl(name, to_lib_mode(mode), error);

#endif
}


file_t file_t::open_or_create (const std::string &name, open_mode mode,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  int create_disposition = is_set(mode, std::ios::trunc)
    ? CREATE_ALWAYS
    : OPEN_ALWAYS;
  return open_impl(name, mode, create_disposition, error);

#else

  auto open_mode = to_lib_mode(mode) | O_CREAT;
  return open_impl(name, open_mode, error);

#endif
}


file_t file_t::unique (std::string &name, std::error_code &error) noexcept
{
#if __sal_os_windows

  if (name.size() < 3)
  {
    name += "XXX";
  }

  std::string dir_name, file_name;
  auto file_pos = name.find_last_of("/\\");
  if (file_pos != name.npos)
  {
    dir_name.assign(name.begin(), name.begin() + file_pos + 1);
    file_name.assign(name.begin() + file_pos + 1, name.end());
  }
  else
  {
    dir_name = ".";
    file_name = name;
  }

  char new_name[MAX_PATH + 1];
  auto result = ::GetTempFileName(dir_name.c_str(),
    file_name.c_str(),
    0,
    new_name
  );
  if (!result)
  {
    error.assign(::GetLastError(), std::system_category());
    return null;
  }

  name = new_name;

  static constexpr auto open_mode = std::ios::in | std::ios::out;
  return open_impl(name, open_mode, CREATE_ALWAYS, error);

#else

  if (name.rfind("XXXXXX") == name.npos)
  {
    name += "XXXXXX";
  }

  auto orig_mode = ::umask(077);
  native_handle handle = ::mkstemp(&name[0]);
  ::umask(orig_mode);

  if (handle == null)
  {
    error.assign(errno, std::generic_category());
  }

  return handle;

#endif
}


void file_t::close (std::error_code &error) noexcept
{
#if __sal_os_windows

  if (handle_ == null)
  {
    error.assign(static_cast<int>(std::errc::bad_file_descriptor),
      std::generic_category()
    );
    return;
  }

  if (::CloseHandle(reinterpret_cast<HANDLE>(handle_)))
  {
    handle_ = null;
    return;
  }

  error.assign(::GetLastError(), std::system_category());

#else

  for (errno = EINTR;  errno == EINTR;  /**/)
  {
    if (::close(handle_) == 0)
    {
      handle_ = null;
      return;
    }
  }

  error.assign(errno, std::generic_category());

#endif
}


size_t file_t::write (const void *data, size_t size, std::error_code &error)
  noexcept
{
#if __sal_os_windows

  DWORD actual_size;
  if (!::WriteFile(reinterpret_cast<HANDLE>(handle_),
      data, static_cast<DWORD>(size),
      &actual_size,
      nullptr))
  {
    error.assign(::GetLastError(), std::system_category());
  }
  return actual_size;

#else

  auto p = static_cast<const char *>(data);
  auto left = size;

  for (errno = EINTR;  left && errno == EINTR;  /**/)
  {
    auto result = ::write(handle_, p, left);
    if (result > -1)
    {
      left -= result;
      p += result;
    }
  }

  if (errno != EINTR)
  {
    error.assign(errno, std::generic_category());
  }

  return size - left;

#endif
}


size_t file_t::read (void *data, size_t size, std::error_code &error) noexcept
{
#if __sal_os_windows

  DWORD actual_size;
  if (!::ReadFile(reinterpret_cast<HANDLE>(handle_),
      data, static_cast<DWORD>(size),
      &actual_size,
      nullptr))
  {
    error.assign(::GetLastError(), std::system_category());
  }
  return actual_size;

#else

  for (errno = EINTR;  errno == EINTR;  /**/)
  {
    auto result = ::read(handle_, data, size);
    if (result > -1)
    {
      return result;
    }
  }

  error.assign(errno, std::generic_category());
  return 0;

#endif
}


int64_t file_t::seek (int64_t offset, seek_dir whence, std::error_code &error)
  noexcept
{
#if __sal_os_windows

  int lib_whence = FILE_CURRENT;
  if (whence == std::ios::beg)
  {
    lib_whence = FILE_BEGIN;
  }
  else if (whence == std::ios::end)
  {
    lib_whence = FILE_END;
  }

  LARGE_INTEGER distance, new_pos;
  distance.QuadPart = offset;

  if (!::SetFilePointerEx(reinterpret_cast<HANDLE>(handle_),
      distance,
      &new_pos,
      lib_whence))
  {
    error.assign(::GetLastError(), std::system_category());
  }

  return new_pos.QuadPart;

#else

  int lib_whence = SEEK_CUR;
  if (whence == std::ios::beg)
  {
    lib_whence = SEEK_SET;
  }
  else if (whence == std::ios::end)
  {
    lib_whence = SEEK_END;
  }

  auto result = ::lseek(handle_, offset, lib_whence);
  if (result == -1)
  {
    error.assign(errno, std::generic_category());
  }

  return result;

#endif
}


__sal_end
