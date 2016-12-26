#include <sal/net/error.hpp>
#include <sal/common.test.hpp>


namespace {


using net_error = sal_test::with_value<sal::net::ip::resolver_errc_t>;


TEST_P(net_error, make_error_code_resolver)
{
  auto error = sal::net::ip::make_error_code(GetParam());

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::ip::resolver_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("resolver", error.category().name());
}


TEST_F(net_error, make_error_code_resolver_invalid)
{
  auto error = sal::net::ip::make_error_code(
    static_cast<sal::net::ip::resolver_errc_t>(-1)
  );

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::ip::resolver_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("resolver", error.category().name());
}


INSTANTIATE_TEST_CASE_P(net_error, net_error,
  ::testing::Values(
    sal::net::ip::resolver_errc_t::host_not_found,
    sal::net::ip::resolver_errc_t::host_not_found_try_again,
    sal::net::ip::resolver_errc_t::service_not_found
  )
);


} // namespace
