#include <sal/logger/__bits/file_sink.hpp>
#include <sal/str.hpp>
#include <sal/time.hpp>


namespace {

// Windows allows both
constexpr char dir_sep = '/';

#if __sal_os_windows

  #include <direct.h>
  #include <windows.h>

  inline constexpr bool is_dir_sep (char ch) noexcept
  {
    return ch == '/' || ch == '\\';
  }

  inline constexpr bool is_drive_sep (char ch) noexcept
  {
    return ch == ':';
  }

  inline unsigned get_this_process_id () noexcept
  {
    return ::GetCurrentProcessId();
  }

  inline int mkdir (const char *dirname, int mode) noexcept
  {
    (void)mode;
    return ::_mkdir(dirname);
  }

#else

  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>

  inline constexpr bool is_dir_sep (char ch) noexcept
  {
    return ch == '/';
  }

  inline constexpr bool is_drive_sep (char ch) noexcept
  {
    (void)ch;
    return false;
  }

  inline unsigned get_this_process_id () noexcept
  {
    return ::getpid();
  }

#endif

} // namespace


namespace sal { namespace logger {
__sal_begin


namespace __bits {


namespace {


void create_directories (std::string dir, str_t<1024> &path)
{
  if (!is_dir_sep(dir.back()))
  {
    dir += dir_sep;
  }

  for (auto ch: dir)
  {
    if (is_dir_sep(ch) && !path.empty() && !is_drive_sep(path.back()))
    {
      if (mkdir(path.data(), 0700) == -1 && errno != EEXIST)
      {
        std::error_code error(errno, std::system_category());
        throw_system_error(error, "create_directories: ", path);
      }
    }
    path << ch;
  }
}


void finish (str_t<event_t::max_message_size> &message) noexcept
{
  if (!message.good())
  {
    // overflow: restore good state and append truncate marker
    // (overwrite last part of message if necessary)
    message.restore();

    static const char marker[] = "<...>";
    if (message.size() + sizeof(marker) > message.max_size())
    {
      // after restoring still doesn't fit, make room for marker
      message.remove_suffix(sizeof(marker) - 1);
    }

    message << marker;
  }

  message << '\n';
}


} // namespace


file_t file_sink_t::make_file ()
{
  auto now = utc_time();
  str_t<1024> filename;

  // dir
  if (!dir_.empty() && dir_ != ".")
  {
    create_directories(dir_, filename);
  }

  // {yyyy}-
  filename << now.tm_year + 1900 << '-';

  // {mm}-
  if (now.tm_mon + 1 < 10) filename << '0';
  filename << now.tm_mon + 1 << '-';

  // {dd}T
  if (now.tm_mday < 10) filename << '0';
  filename << now.tm_mday << 'T';

  // {HH}
  if (now.tm_hour < 10) filename << '0';
  filename << now.tm_hour;

  // {MM}
  if (now.tm_min < 10) filename << '0';
  filename << now.tm_min;

  // _{label}.log
  filename << suffix_;

  auto file = file_t::open_or_create(filename.data(),
    std::ios::out | std::ios::app
  );

  // add header to file
  str_t<1024> header;
  header
    << "\n#"
    << "\n# log=" << filename << ';'
    << "\n# pid=" << get_this_process_id() << ';'
    << "\n#\n\n";
  file.write(header.data(), header.size());

  return file;
}


void file_sink_t::event_write (event_t &event)
{
  finish(event.message);

  if (buffer_)
  {
    if (buffer_->size() + event.message.size() > buffer_->capacity())
    {
      flush();
    }
    buffer_->append(event.message.cbegin(), event.message.cend());
  }
  else
  {
    file_.write(event.message.data(), event.message.size());
  }
}


} // namespace __bits


__sal_end
}} // namespace sal::logger
