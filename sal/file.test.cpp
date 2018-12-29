#include <sal/file.hpp>
#include <sal/common.test.hpp>
#include <sal/span.hpp>
#include <fstream>


namespace {


using file = sal_test::fixture;


constexpr std::ios::openmode in_out = std::ios::in | std::ios::out;
const std::string inaccessible_file_name =
#if __sal_os_windows
  "C:\\"
#else
  "/"
#endif
  "sal_test.XXXXXX";


std::string create_random_file (const std::string &test_name)
{
  std::string name = "sal_test." + test_name + ".XXXXXX";

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::unique(name));
  EXPECT_TRUE(file.is_open());

  return name;
}


TEST_F(file, ctor)
{
  sal::file_t file;
  EXPECT_FALSE(file.is_open());
}


TEST_F(file, create_success)
{
  auto name = create_random_file(case_name);
  std::remove(name.c_str());

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::create(name, in_out));
  EXPECT_TRUE(file.is_open());
  file.close();

  std::remove(name.c_str());
}


TEST_F(file, create_fail)
{
  auto name = create_random_file(case_name);

  sal::file_t file;
  EXPECT_THROW(file = sal::file_t::create(name, in_out), std::system_error);
  EXPECT_FALSE(file.is_open());

  std::remove(name.c_str());
}


TEST_F(file, create_no_mode)
{
  auto name = create_random_file(case_name);
  std::remove(name.c_str());

  std::ios::openmode no_mode{};
  auto file = sal::file_t::create(name, no_mode);
  EXPECT_TRUE(file.is_open());

  std::remove(name.c_str());
}


TEST_F(file, open_success)
{
  auto name = create_random_file(case_name);

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open(name, in_out));
  EXPECT_TRUE(file.is_open());
  file.close();

  std::remove(name.c_str());
}


TEST_F(file, open_fail)
{
  auto name = create_random_file(case_name);
  std::remove(name.c_str());

  sal::file_t file;
  EXPECT_THROW(file = sal::file_t::open(name, in_out), std::system_error);
  EXPECT_FALSE(file.is_open());
}


TEST_F(file, open_or_create_success_open)
{
  auto name = create_random_file(case_name);

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open_or_create(name, in_out));
  EXPECT_TRUE(file.is_open());
  file.close();

  std::remove(name.c_str());
}


TEST_F(file, open_or_create_success_create)
{
  auto name = create_random_file(case_name);
  std::remove(name.c_str());

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open_or_create(name, in_out));
  EXPECT_TRUE(file.is_open());
  file.close();

  std::remove(name.c_str());
}


TEST_F(file, open_or_create_fail_permission_denied)
{
  sal::file_t file;
  EXPECT_THROW(
    file = sal::file_t::open_or_create(inaccessible_file_name, in_out);
    , std::system_error
  );
  EXPECT_FALSE(file.is_open());
}


TEST_F(file, unique_success)
{
  std::string name = "sal_test." + case_name + ".XXXXXX";

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::unique(name));
  EXPECT_TRUE(file.is_open());
  file.close();

  struct stat info;
  EXPECT_NE(::stat(name.c_str(), &info), -1);

  std::remove(name.c_str());
}


TEST_F(file, unique_missing_pattern_success)
{
  std::string name = "sal_test." + case_name;

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::unique(name));
  EXPECT_TRUE(file.is_open());
  file.close();

  EXPECT_NE("", name);

  std::remove(name.c_str());
}


TEST_F(file, unique_fail_permission_denied)
{
  sal::file_t file;
  auto name = inaccessible_file_name;
  EXPECT_THROW(file = sal::file_t::unique(name), std::system_error);
  EXPECT_FALSE(file.is_open());
}


TEST_F(file, close_success)
{
  std::string name = "sal_test." + case_name + ".XXXXXX";
  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::unique(name));
  EXPECT_TRUE(file.is_open());
  file.close();
  EXPECT_FALSE(file.is_open());

  std::remove(name.c_str());
}


TEST_F(file, close_fail)
{
  sal::file_t file;
  EXPECT_THROW(file.close(), std::system_error);
  EXPECT_FALSE(file.is_open());
}


TEST_F(file, swap)
{
  auto name = create_random_file(case_name);

  auto a = sal::file_t::open(name, in_out);
  EXPECT_TRUE(a.is_open());

  using std::swap;
  sal::file_t b;
  swap(a, b);

  EXPECT_FALSE(a.is_open());
  EXPECT_TRUE(b.is_open());
  b.close();

  std::remove(name.c_str());
}


TEST_F(file, move_to_closed)
{
  auto name = create_random_file(case_name);

  auto a = sal::file_t::open(name, in_out);
  EXPECT_TRUE(a.is_open());

  auto b = std::move(a);

  EXPECT_FALSE(a.is_open());
  EXPECT_TRUE(b.is_open());
  b.close();

  std::remove(name.c_str());
}


TEST_F(file, move_to_opened)
{
  auto a_name = create_random_file(case_name);
  auto a = sal::file_t::open(a_name, in_out);
  EXPECT_TRUE(a.is_open());

  auto b_name = create_random_file(case_name);
  auto b = sal::file_t::open(a_name, in_out);
  EXPECT_TRUE(b.is_open());

  b = std::move(a);

  EXPECT_FALSE(a.is_open());
  EXPECT_TRUE(b.is_open());
  b.close();

  std::remove(a_name.c_str());
  std::remove(b_name.c_str());
}


TEST_F(file, write_out_success)
{
  auto name = create_random_file(case_name);

  std::string first(name.begin(), name.begin() + name.size()/2);
  std::string second(name.begin() + name.size()/2, name.end());
  second += '\n';

  {
    sal::file_t file;
    EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::out));
    EXPECT_TRUE(file.is_open());
    EXPECT_EQ(first.size(), file.write(first));
    EXPECT_EQ(second.size(), file.write(second));
  }

  std::ifstream fin{name};
  std::string line;
  EXPECT_TRUE(std::getline(fin, line).good());
  EXPECT_EQ(name, line);
  EXPECT_TRUE(std::getline(fin, line).eof());
  fin.close();

  std::remove(name.c_str());
}


TEST_F(file, write_trunc_success)
{
  auto name = create_random_file(case_name);

  {
    sal::file_t file;
    EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::out));
    EXPECT_TRUE(file.is_open());
    auto d = case_name + "first\n";
    EXPECT_EQ(d.size(), file.write(d));
  }

  std::ifstream fin{name};
  std::string line;
  EXPECT_TRUE(std::getline(fin, line).good());
  EXPECT_EQ(case_name + "first", line);
  EXPECT_TRUE(std::getline(fin, line).eof());
  fin.close();

  {
    sal::file_t file;
    EXPECT_NO_THROW(
      file = sal::file_t::open(name, std::ios::out | std::ios::trunc)
    );
    EXPECT_TRUE(file.is_open());
    auto d = case_name + "second\n";
    EXPECT_EQ(d.size(), file.write(d));
  }

  fin.open(name);
  EXPECT_TRUE(std::getline(fin, line).good());
  EXPECT_EQ(case_name + "second", line);
  EXPECT_TRUE(std::getline(fin, line).eof());
  fin.close();

  std::remove(name.c_str());
}


TEST_F(file, write_append_success)
{
  auto name = create_random_file(case_name);

  {
    sal::file_t file;
    EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::out));
    EXPECT_TRUE(file.is_open());
    auto d = case_name + "first\n";
    EXPECT_EQ(d.size(), file.write(d));
  }

  std::ifstream fin{name};
  std::string line;
  EXPECT_TRUE(std::getline(fin, line).good());
  EXPECT_EQ(case_name + "first", line);
  fin.close();

  {
    sal::file_t file;
    EXPECT_NO_THROW(
      file = sal::file_t::open(name, std::ios::out | std::ios::app)
    );
    EXPECT_TRUE(file.is_open());
    auto d = case_name + "second\n";
    EXPECT_EQ(d.size(), file.write(d));
  }

  fin.open(name);
  EXPECT_TRUE(std::getline(fin, line).good());
  EXPECT_EQ(case_name + "first", line);
  EXPECT_TRUE(std::getline(fin, line).good());
  EXPECT_EQ(case_name + "second", line);
  EXPECT_TRUE(std::getline(fin, line).eof());
  fin.close();

  std::remove(name.c_str());
}


TEST_F(file, write_in_fail)
{
  auto name = create_random_file(case_name);

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::in));
  EXPECT_TRUE(file.is_open());

  EXPECT_THROW(file.write(case_name), std::system_error);

  file.close();

  std::remove(name.c_str());
}


TEST_F(file, write_closed_fail)
{
  auto name = create_random_file(case_name);

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::out));
  EXPECT_TRUE(file.is_open());
  file.close();
  EXPECT_FALSE(file.is_open());

  EXPECT_THROW(file.write(case_name), std::system_error);

  std::remove(name.c_str());
}


TEST_F(file, read_in_success)
{
  auto name = create_random_file(case_name);
  std::ofstream(name) << case_name;

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::in));
  EXPECT_TRUE(file.is_open());

  char line[1024];
  EXPECT_EQ(case_name.size(), file.read(line));
  EXPECT_EQ(case_name, std::string(line, line + case_name.size()));
  file.close();

  std::remove(name.c_str());
}


TEST_F(file, read_eof_success)
{
  auto name = create_random_file(case_name);
  std::ofstream(name) << case_name;

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::in));
  EXPECT_TRUE(file.is_open());

  char line[1024];
  EXPECT_EQ(case_name.size(), file.read(line));
  EXPECT_EQ(case_name, std::string(line, line + case_name.size()));

  std::error_code error;
  EXPECT_EQ(0U, file.read(line, error));
  EXPECT_FALSE(error);

  file.close();

  std::remove(name.c_str());
}


TEST_F(file, read_out_fail)
{
  auto name = create_random_file(case_name);
  std::ofstream(name) << case_name;

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::out));
  EXPECT_TRUE(file.is_open());

  char line[1024];
  EXPECT_THROW(file.read(line), std::system_error);
  file.close();

  std::remove(name.c_str());
}


TEST_F(file, seek_success)
{
  auto name = create_random_file(case_name);
  std::ofstream(name, std::ios::binary) << "xxxx\n";

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::out));
  EXPECT_TRUE(file.is_open());

  int64_t file_pos;
  EXPECT_NO_THROW(file_pos = file.seek(-3, std::ios::end));
  EXPECT_EQ(2, file_pos);
  EXPECT_EQ(2U, file.write(sal::span("st", 2)));

  EXPECT_NO_THROW(file_pos = file.seek(1, std::ios::beg));
  EXPECT_EQ(1, file_pos);
  EXPECT_EQ(1U, file.write(sal::span("e", 1)));

  EXPECT_NO_THROW(file_pos = file.seek(-2, std::ios::cur));
  EXPECT_EQ(0, file_pos);
  EXPECT_EQ(1U, file.write(sal::span("t", 1)));

  file.close();

  std::ifstream fin{name};
  std::string line;
  EXPECT_TRUE(std::getline(fin, line).good());
  EXPECT_EQ("test", line);
  fin.close();

  std::remove(name.c_str());
}


TEST_F(file, seek_past_end_success)
{
  auto name = create_random_file(case_name);
  std::ofstream(name) << "te";

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open(name, in_out));
  EXPECT_TRUE(file.is_open());

  int64_t file_pos;
  EXPECT_NO_THROW(file_pos = file.seek(0, std::ios::end));
  EXPECT_EQ(2, file_pos);

  EXPECT_NO_THROW(file_pos = file.seek(2, std::ios::cur));
  EXPECT_EQ(4, file_pos);
  EXPECT_NO_THROW(file.write(sal::span("\n", 1)));

  EXPECT_NO_THROW(file_pos = file.seek(0, std::ios::beg));
  EXPECT_EQ(0, file_pos);

  char data[1024];
  size_t result;
  EXPECT_NO_THROW(result = file.read(data));
  EXPECT_EQ(5U, result);
  EXPECT_EQ('t', data[0]);
  EXPECT_EQ('e', data[1]);
  EXPECT_EQ('\0', data[2]);
  EXPECT_EQ('\0', data[3]);
  EXPECT_EQ('\n', data[4]);

  file.close();
  std::remove(name.c_str());
}


TEST_F(file, seek_before_beg_fails)
{
  auto name = create_random_file(case_name);

  sal::file_t file;
  EXPECT_NO_THROW(file = sal::file_t::open(name, std::ios::in));
  EXPECT_TRUE(file.is_open());

  int64_t file_pos;
  EXPECT_NO_THROW(file_pos = file.seek(0, std::ios::beg));
  EXPECT_EQ(0, file_pos);

  EXPECT_THROW(file.seek(-1, std::ios::beg), std::system_error);

  file.close();
  std::remove(name.c_str());
}


} // namespace
