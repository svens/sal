#include <sal/error.hpp>
#include <sal/common.test.hpp>


namespace {


using error = sal_test::fixture;


TEST_F(error, logic_error)
{
  bool visited = false;
  try
  {
    sal::throw_logic_error(case_name, 42);
  }
  catch (const std::logic_error &e)
  {
    visited = true;
    EXPECT_EQ(case_name + "42", e.what());
  }
  catch (...)
  {
    ASSERT_FALSE(true) << "shouldn't reach here";
  }
  EXPECT_TRUE(visited);
}


TEST_F(error, runtime_error)
{
  bool visited = false;
  try
  {
    sal::throw_runtime_error(case_name, 42);
  }
  catch (const std::runtime_error &e)
  {
    visited = true;
    EXPECT_EQ(case_name + "42", e.what());
  }
  catch (...)
  {
    ASSERT_FALSE(true) << "shouldn't reach here";
  }
  EXPECT_TRUE(visited);
}


TEST_F(error, system_error)
{
  const auto code = std::make_error_code(std::errc::not_enough_memory);

  bool visited = false;
  try
  {
    sal::throw_system_error(code, case_name, 42);
  }
  catch (const std::system_error &e)
  {
    visited = true;
    EXPECT_NE(std::string::npos,
      std::string(e.what()).find(case_name + "42")
    );
    EXPECT_NE(std::string::npos,
      std::string(e.what()).find(code.message())
    );
  }
  catch (...)
  {
    ASSERT_FALSE(true) << "shouldn't reach here";
  }
  EXPECT_TRUE(visited);
}


} // namespace
