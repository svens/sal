#include <sal/logger/file_sink.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/common.test.hpp>
#include <sal/error.hpp>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <vector>


#if __sal_os_windows
  #include <direct.h>
  #include <io.h>
  #define rmdir _rmdir
#else
  #include <dirent.h>
  #include <unistd.h>
#endif


namespace sal_test {


#if __sal_os_windows

const std::string permission_denied_dir = "C:/Windows";

std::vector<std::string> directory_listing (const std::string &path)
{
  auto filespec = path + "/*";
  std::vector<std::string> result;

  struct ::_finddata_t file_info;
  auto h = ::_findfirst(filespec.c_str(), &file_info);
  if (h == -1)
  {
    return result;
  }

  do
  {
    auto name = std::string(file_info.name);
    if (name != "." && name != "..")
    {
      result.push_back(path + '\\' + name);
    }
  } while (::_findnext(h, &file_info) == 0);

  ::_findclose(h);

  std::sort(result.begin(), result.end());
  return result;
}

#else

const std::string permission_denied_dir = "/";

std::vector<std::string> directory_listing (const std::string &path)
{
  std::vector<std::string> result;
  if (auto dir = ::opendir(path.c_str()))
  {
    while (auto it = ::readdir(dir))
    {
      auto name = std::string(it->d_name);
      if (name != "." && name != "..")
      {
        result.push_back(path + '/' + name);
      }
    }
    ::closedir(dir);
  }
  std::sort(result.begin(), result.end());
  return result;
}

#endif


std::string read_file (const std::string &name)
{
  std::ifstream file(name);
  if (file.fail())
  {
    sal::throw_system_error(
      std::error_code(errno, std::system_category()),
      "read_file: ", name
    );
  }

  std::string line, content;
  while (std::getline(file, line))
  {
    content += line;
    content += '\n';
  }

  return content;
}


bool file_contains (const std::string &needle, const std::string &file)
{
  return read_file(file).find(needle) != std::string::npos;
}


} // sal_test


namespace {


using namespace sal_test;


template <typename Worker>
struct logger_file_sink
  : public sal_test::with_type<Worker>
{
  const std::string test_logs = "test_logs";
  std::unique_ptr<Worker> worker_ = std::make_unique<Worker>();


  template <typename... Options>
  auto make_channel (Options &&...options)
  {
    auto sink = sal::logger::file("test",
      sal::logger::set_file_dir(test_logs),
      std::forward<Options>(options)...
    );
    auto channel_name = this->case_name;
    std::reverse(channel_name.begin(), channel_name.end());
    return worker_->make_channel(channel_name,
      sal::logger::set_channel_sink(sink)
    );
  }


  void stop_and_close_logs ()
  {
    worker_.reset();
  }


  auto log_files ()
  {
    return directory_listing(test_logs);
  }


  void SetUp () override
  {
    // remove test_logs before starting testing
    for (auto &file: directory_listing(test_logs))
    {
      std::remove(file.c_str());
    }
    ::rmdir(test_logs.c_str());
  }


  void TearDown () override
  {
    stop_and_close_logs();
    SetUp();
  }

};

using worker_types = testing::Types<
  sal::logger::worker_t,
  sal::logger::async_worker_t
>;

TYPED_TEST_CASE(logger_file_sink, worker_types);


TYPED_TEST(logger_file_sink, log)
{
  auto channel = this->make_channel();
  sal_log(channel) << this->case_name;
  this->stop_and_close_logs();

  auto log_files = this->log_files();
  EXPECT_EQ(1U, log_files.size());
  EXPECT_TRUE(file_contains(this->case_name, log_files[0]));
}


TYPED_TEST(logger_file_sink, log_buffered)
{
  auto channel = this->make_channel(sal::logger::set_file_buffer_size_kb(1));
  sal_log(channel) << this->case_name;
  for (auto i = 0;  i != 10;  ++i)
  {
    // log 10 long messages to make sure we hit full buffer condition
    sal_log(channel) << std::string(sal::logger::event_t::max_message_size/2, 'x');
  }
  this->stop_and_close_logs();

  auto log_files = this->log_files();
  EXPECT_EQ(1U, log_files.size());
  EXPECT_TRUE(file_contains(this->case_name, log_files[0]));
}


TYPED_TEST(logger_file_sink, log_overflow)
{
  auto channel = this->make_channel();
  std::string overflowed_message(sal::logger::event_t::max_message_size, 'x');
  sal_log(channel) << this->case_name << ' ' << overflowed_message;
  this->stop_and_close_logs();

  auto log_files = this->log_files();
  EXPECT_EQ(1U, log_files.size());
  auto log_content = read_file(log_files[0]);

  EXPECT_EQ(log_content.npos, log_content.find(this->case_name));
  EXPECT_EQ(log_content.npos, log_content.find(overflowed_message));
  EXPECT_NE(log_content.npos, log_content.find("<...>"));
}


TYPED_TEST(logger_file_sink, local_time)
{
  auto channel = this->make_channel(sal::logger::set_file_utc_time(false));
  sal_log(channel) << this->case_name;
  this->stop_and_close_logs();

  auto log_files = this->log_files();
  EXPECT_EQ(1U, log_files.size());
  auto log_content = read_file(log_files[0]);

  // check for logged message
  EXPECT_NE(log_content.npos, log_content.find(this->case_name));

  //
  // check for timestamp (at beginning of line i.e. prepend '\n')
  //

  auto now = sal::now();
  auto tm = sal::local_time(now);
  char buf[64];

  // in current second
  snprintf(buf, sizeof(buf), "\n%02u:%02u:%02u",
    tm.tm_hour, tm.tm_min, tm.tm_sec
  );
  bool found = log_content.npos != log_content.find(std::string(buf));

  // if not found, then in previous second as well
  if (!found)
  {
    using namespace std::chrono_literals;
    tm = sal::local_time(now - 1s);
    snprintf(buf, sizeof(buf), "\n%02u:%02u:%02u",
      tm.tm_hour, tm.tm_min, tm.tm_sec
    );
    found = log_content.npos != log_content.find(std::string(buf));
  }

  EXPECT_TRUE(found);
}


TYPED_TEST(logger_file_sink, utc_time)
{
  auto channel = this->make_channel(sal::logger::set_file_utc_time(true));
  sal_log(channel) << this->case_name;
  this->stop_and_close_logs();

  auto log_files = this->log_files();
  EXPECT_EQ(1U, log_files.size());
  auto log_content = read_file(log_files[0]);

  // check for logged message
  EXPECT_NE(log_content.npos, log_content.find(this->case_name));

  //
  // check for timestamp
  //

  auto now = sal::now();
  auto tm = sal::utc_time(now);
  char buf[64];

  // in current second
  snprintf(buf, sizeof(buf), "\n%02u:%02u:%02u",
    tm.tm_hour, tm.tm_min, tm.tm_sec
  );
  bool found = log_content.npos != log_content.find(std::string(buf));

  // if not found, then in previous second as well
  if (!found)
  {
    using namespace std::chrono_literals;
    tm = sal::utc_time(now - 1s);
    snprintf(buf, sizeof(buf), "\n%02u:%02u:%02u",
      tm.tm_hour, tm.tm_min, tm.tm_sec
    );
    found = log_content.npos != log_content.find(std::string(buf));
  }

  EXPECT_TRUE(found);
}


TYPED_TEST(logger_file_sink, max_size)
{
  // here we use internal knowledge how to configure max log file size
  // (don't want to use public API with 1MB precision)

  auto channel = this->make_channel(
    sal::logger::__bits::file_max_size(1024)
  );
  for (auto i = 0;  i < 100;  ++i)
  {
    sal_log(channel) << this->case_name << '_' << (i + 1);
  }
  this->stop_and_close_logs();

  auto log_files = this->log_files();
  EXPECT_LT(1U, log_files.size());
  EXPECT_TRUE(file_contains(this->case_name + "_1\n", log_files[0]));
  EXPECT_TRUE(file_contains(this->case_name + "_100\n", log_files.back()));
}


TYPED_TEST(logger_file_sink, new_day)
{
  auto channel = this->make_channel();
  sal_log(channel) << "old day";
  {
    using namespace std::chrono_literals;
    auto event = channel.make_event();
    event->time += 24h;
    event->message << "new day";
  }
  this->stop_and_close_logs();

  auto log_files = this->log_files();
  EXPECT_LE(1U, log_files.size());
  EXPECT_TRUE(file_contains("old day", log_files[0]));
  EXPECT_TRUE(file_contains("new day", log_files.back()));
}


TYPED_TEST(logger_file_sink, unprivileged_dir)
{
  EXPECT_THROW(
    this->make_channel(sal::logger::set_file_dir(permission_denied_dir)),
    std::system_error
  );
}


} // namespace
