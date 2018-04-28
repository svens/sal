#include <sal/net/error.hpp>
#include <sal/common.test.hpp>


namespace {


using resolver_error = sal_test::with_value<sal::net::ip::resolver_errc>;


TEST_P(resolver_error, make_error_code)
{
  auto error = sal::net::ip::make_error_code(GetParam());

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::ip::resolver_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("resolver", error.category().name());
}


TEST_F(resolver_error, make_error_code_invalid)
{
  auto error = sal::net::ip::make_error_code(
    static_cast<sal::net::ip::resolver_errc>(-1)
  );

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::ip::resolver_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("resolver", error.category().name());
}


INSTANTIATE_TEST_CASE_P(net, resolver_error,
  ::testing::Values(
    sal::net::ip::resolver_errc::host_not_found,
    sal::net::ip::resolver_errc::host_not_found_try_again,
    sal::net::ip::resolver_errc::service_not_found
  ),
);


using socket_error = sal_test::with_value<sal::net::socket_errc>;


TEST_P(socket_error, make_error_code)
{
  auto error = sal::net::make_error_code(GetParam());

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::socket_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("socket", error.category().name());
}


TEST_F(socket_error, make_error_code_invalid)
{
  auto error = sal::net::make_error_code(
    static_cast<sal::net::socket_errc>(-1)
  );

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::socket_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("socket", error.category().name());
}


INSTANTIATE_TEST_CASE_P(net, socket_error,
  ::testing::Values(
    sal::net::socket_errc::already_open,
    sal::net::socket_errc::already_associated
  ),
);


} // namespace
