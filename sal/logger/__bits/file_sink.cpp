#include <sal/logger/__bits/file_sink.hpp>
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


__sal_begin


namespace logger { namespace __bits {


namespace {


using path_t = char_array_t<1024>;


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
      if (mkdir(path.c_str(), 0700) == -1 && errno != EEXIST)
      {
        throw_system_error(std::error_code(errno, std::system_category()),
          "create_directories: ",
          path
        );
      }
    }
    path << ch;
  }
}


void make_filename (path_t &filename,
  const std::tm &tm,
  const std::string &suffix) noexcept
{
  // {yyyy}-
  filename << tm.tm_year + 1900 << '-';

  // LCOV_EXCL_BR_START

  // {mm}-
  if (tm.tm_mon + 1 < 10) filename << '0';
  filename << tm.tm_mon + 1 << '-';

  // {dd}T
  if (tm.tm_mday < 10) filename << '0';
  filename << tm.tm_mday << 'T';

  // {HH}
  if (tm.tm_hour < 10) filename << '0';
  filename << tm.tm_hour;

  // {MM}
  if (tm.tm_min < 10) filename << '0';
  filename << tm.tm_min;

  // {SS}
  if (tm.tm_sec < 10) filename << '0';
  filename << tm.tm_sec;

  // LCOV_EXCL_BR_STOP

  // _{label}.log
  filename << suffix;
}


size_t get_size_and_filename (path_t &filename, size_t max_size) noexcept
{
  // check up to 1000 files
  for (size_t i = 0;  i < 1000;  ++i)
  {
    struct stat st;
    if (::stat(filename.c_str(), &st) == 0)
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
    // LCOV_EXCL_BR_START
    if (i > 100) filename.remove_suffix(4);
    else if (i > 10) filename.remove_suffix(3);
    else if (i > 0) filename.remove_suffix(2);
    // LCOV_EXCL_BR_STOP
    filename << '.' << i;
  }

  // couldn't find any file in current second that can fit more messages
  // nothing we can do, keep appending to last file (and lie about size to
  // postpone next size check into next second hopefully; 10% of max size)
  return max_size / 10;
}


void finish (char_array_t<event_t::max_message_size> &message) noexcept
{
  if (!message.good())
  {
    static constexpr const char marker[] = "<...>";
    message.reset();
    message << marker;
  }
  message << '\n';
}


inline uint8_t today (time_t time) noexcept
{
  // time is UTC or local
  // doing utc_time on top of that does not affect it
  return static_cast<uint8_t>(utc_time(time).tm_mday);
}


bool new_day_started (time_t time) noexcept
{
  using namespace std::chrono_literals;

  constexpr auto interval = 1s;
  static auto next_check = time + interval;
  static auto day = today(time);

  if (sal_unlikely(time >= next_check))
  {
    static spinlock_t mutex;
    std::lock_guard<spinlock_t> lock(mutex);

    if (time >= next_check)
    {
      next_check += interval;

      const auto new_day = today(time);
      if (day != new_day)
      {
        day = new_day;
        return true;
      }
    }
  }

  return false;
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
  auto tm = utc_time_ ? utc_time() : local_time();
  make_filename(filename, tm, suffix_);

  // next filename index which size < max_size
  if (max_size_)
  {
    size_ = get_size_and_filename(filename, max_size_);
  }

  auto file = file_t::open_or_create(filename.c_str(),
    std::ios::out | std::ios::app
  );

  // add header to file
  char_array_t<1024> header;
  header
    << "\n#"
    << "\n# log=" << filename << ';'
    << "\n# pid=" << get_this_process_id() << ';'
    << "\n#\n\n";
  size_ += file.write(make_buf(header));

  return file;
}


void file_sink_t::sink_event_write (event_t &event)
{
  finish(event.message);

  lock_t lock(mutex_);

  // rotate file if necessary
  if (new_day_started(event.time))
  {
    rotate();
  }
  else if (max_size_)
  {
    if (size_ + event.message.size() > max_size_)
    {
      rotate();
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
    buffer_->append(event.message.begin(), event.message.end());
  }
  else
  {
    file_.write(make_buf(event.message));
  }
}


}} // namespace logger::__bits


__sal_end
