#include <sal/service/service_base.hpp>
#include <sal/logger/logger.hpp>
#include <sal/service/mock.test.hpp>
#include <sal/common.test.hpp>
#include <sal/logger/common.test.hpp>
#include <cstdio>
#include <thread>


namespace {


using namespace sal_test;
using namespace std::chrono_literals;


struct service_base
  : public sal_test::fixture
{
  template <size_t N>
  auto make_service (const char *(&&args)[N])
  {
    return sal::service::service_base_t(
      static_cast<int>(std::size(args)),
      args,
      {}
    );
  }

  auto make_service ()
  {
    return make_service({"svc"});
  }
};


using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::SaveArg;
using ::testing::Throw;


TEST_F(service_base, help)
{
  auto svc = make_service({"svc", "--help"});
  EXPECT_TRUE(svc.help_requested());

  std::ostringstream oss;
  EXPECT_EQ(EXIT_SUCCESS, svc.help(oss));
  auto help = oss.str();
  EXPECT_NE(help.npos, help.find("svc"));
  EXPECT_NE(help.npos, help.find("help"));
  EXPECT_NE(help.npos, help.find("service.logger.dir"));
  EXPECT_NE(help.npos, help.find("service.logger.sink"));
  EXPECT_NE(help.npos, help.find("service.thread_count"));
}


TEST_F(service_base, logger_stdout)
{
  std::ostringstream oss;
  auto rdbuf = std::cout.rdbuf(oss.rdbuf());
  {
    auto svc = make_service({"svc", "--service.logger.sink=stdout"});
    sal_log(svc) << case_name;
  }
  std::cout.rdbuf(rdbuf);

  auto message = oss.str();
  EXPECT_NE(message.npos, message.find(case_name));
}


TEST_F(service_base, logger_null)
{
  std::ostringstream oss;
  auto rdbuf = std::cout.rdbuf(oss.rdbuf());
  {
    auto svc = make_service({"svc", "--service.logger.sink=null"});
    sal_log(svc) << case_name;
  }
  std::cout.rdbuf(rdbuf);

  EXPECT_TRUE(oss.str().empty());
}


void clean_logs (const std::string &dir)
{
  for (auto &file: directory_listing(dir))
  {
    std::remove(file.c_str());
  }
#if __sal_os_windows
  ::_rmdir(dir.c_str());
#else
  ::rmdir(dir.c_str());
#endif
}


TEST_F(service_base, logger_file)
{
  clean_logs("logs");

  {
    auto svc = make_service({
      "svc",
      "--service.logger.dir=logs",
      "--service.logger.sink=svc"
    });
    sal_log(svc) << case_name;
  }

  auto logs = directory_listing("logs");
  EXPECT_FALSE(logs.empty());
  EXPECT_TRUE(file_contains(case_name, logs[0]));

  clean_logs("logs");
}


TEST_F(service_base, thread_count_default)
{
  auto svc = make_service();
  EXPECT_EQ(std::thread::hardware_concurrency(), svc.config.thread_count);
}


TEST_F(service_base, thread_count)
{
  auto svc = make_service({"svc", "--service.thread_count=1"});
  EXPECT_EQ(1U, svc.config.thread_count);
}


TEST_F(service_base, thread_count_zero)
{
  EXPECT_THROW(
    make_service({"svc", "--service.thread_count=0"}),
    std::runtime_error
  );
}


TEST_F(service_base, thread_count_invalid)
{
  EXPECT_THROW(
    make_service({"svc", "--service.thread_count=X"}),
    std::runtime_error
  );
}


TEST_F(service_base, exit_during_start)
{
  auto svc = make_service();
  ::testing::NiceMock<service_event_handler_mock_t> event_handler;

  ON_CALL(event_handler, service_start(_))
    .WillByDefault(
      InvokeWithoutArgs([&]()
      {
        svc.exit(EXIT_FAILURE);
      })
    );

  EXPECT_CALL(event_handler, service_stop());

  EXPECT_EQ(EXIT_FAILURE, svc.run(event_handler, 10ms));
}


TEST_F(service_base, exit_during_tick)
{
  auto svc = make_service();
  ::testing::NiceMock<service_event_handler_mock_t> event_handler;

  ON_CALL(event_handler, service_tick(_))
    .WillByDefault(
      InvokeWithoutArgs([&]()
      {
        svc.exit(EXIT_FAILURE);
      })
    );

  EXPECT_CALL(event_handler, service_stop());

  EXPECT_EQ(EXIT_FAILURE, svc.run(event_handler));
}


TEST_F(service_base, throw_during_start)
{
  auto svc = make_service();
  ::testing::NiceMock<service_event_handler_mock_t> event_handler;

  ON_CALL(event_handler, service_start(_))
    .WillByDefault(Throw(std::runtime_error(case_name)));

  EXPECT_CALL(event_handler, service_stop());

  try
  {
    svc.run(event_handler, 10ms);
    FAIL();
  }
  catch (const std::runtime_error &e)
  {
    EXPECT_EQ(case_name, e.what());
  }
}


TEST_F(service_base, throw_during_tick)
{
  auto svc = make_service();
  ::testing::NiceMock<service_event_handler_mock_t> event_handler;

  ON_CALL(event_handler, service_tick(_))
    .WillByDefault(Throw(std::runtime_error(case_name)));

  EXPECT_CALL(event_handler, service_stop());

  try
  {
    svc.run(event_handler);
    FAIL();
  }
  catch (const std::runtime_error &e)
  {
    EXPECT_EQ(case_name, e.what());
  }
}


TEST_F(service_base, throw_during_stop)
{
  auto svc = make_service();
  ::testing::NiceMock<service_event_handler_mock_t> event_handler;

  ON_CALL(event_handler, service_tick(_))
    .WillByDefault(
      InvokeWithoutArgs([&]()
      {
        svc.exit(EXIT_SUCCESS);
      })
    );

  ON_CALL(event_handler, service_stop())
    .WillByDefault(Throw(std::runtime_error(case_name)));

  try
  {
    svc.run(event_handler);
    FAIL();
  }
  catch (const std::runtime_error &e)
  {
    EXPECT_EQ(case_name, e.what());
  }
}


TEST_F(service_base, tick_frequency)
{
  auto svc = make_service();
  ::testing::NiceMock<service_event_handler_mock_t> event_handler;

  sal::time_t first{}, second{};
  EXPECT_CALL(event_handler, service_tick(_))
    .WillOnce(SaveArg<0>(&first))
    .WillOnce(
      DoAll(
        SaveArg<0>(&second),
        InvokeWithoutArgs([&]()
        {
          svc.exit(EXIT_SUCCESS);
        })
      )
    );

  constexpr auto tick_interval = 10ms;
  EXPECT_EQ(EXIT_SUCCESS, svc.run(event_handler, tick_interval));
  EXPECT_LE(first + tick_interval, second);
}


} // namespace
