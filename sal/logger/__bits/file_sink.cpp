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

  inline int mkdir (const char *dirname, int /*mode*/) noexcept
  {
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

  inline constexpr bool is_drive_sep (char /*ch*/) noexcept
  {
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


using path_t = str_t<1024>;


void create_directories (std::string dir, path_t &path)
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


void make_filename (path_t &filename, const std::string &suffix) noexcept
{
  const auto now = utc_time();

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

  // {SS}
  if (now.tm_sec < 10) filename << '0';
  filename << now.tm_sec;

  // _{label}.log
  filename << suffix;
}


size_t get_size_and_filename (path_t &filename, size_t max_size) noexcept
{
  // check up to 1000 files
  for (size_t i = 0;  i < 1000;  ++i)
  {
    struct stat st;
    if (::stat(filename.data(), &st) == 0)
    {
      if (st.st_size + event_t::max_message_size < max_size)
      {
        // exists and has room for at least one maximum size message
        return st.st_size;
      }
    }
    else if (errno == ENOENT)
    {
      // does not exist
      return 0;
    }
    // else: existing file size exceeds max_size already

    // add/replace index in current filename and try again
    if (i > 100) filename.remove_suffix(4);
    else if (i > 10) filename.remove_suffix(3);
    else if (i > 0) filename.remove_suffix(2);
    filename << '.' << i;
  }

  // couldn't find any file in current second that can fit more messages
  // nothing we can do, keep appending to last file (and lie about size to
  // postpone next size check into next second hopefully)
  return max_size / 2;
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
  path_t filename;

  // dir
  if (!dir_.empty() && dir_ != ".")
  {
    create_directories(dir_, filename);
  }

  // filename
  make_filename(filename, suffix_);

  // next filename index which size < max_size
  if (max_size_)
  {
    size_ = get_size_and_filename(filename, max_size_);
  }

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
  size_ += file.write(header.data(), header.size());

  return file;
}


void file_sink_t::event_write (event_t &event)
{
  // rotate at change of day
  // TODO

  finish(event.message);

  // rotate file if writing new message would exceed max size
  if (max_size_)
  {
    if (size_ + event.message.size() > max_size_)
    {
      if (auto file = make_file())
      {
        flush();
        swap_file(file);
      }
    }
    size_ += event.message.size();
  }

  // write (or buffer/flush)
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
