#include <sal/uri/scheme.hpp>
#include <sal/uri/uri.hpp>
#include <sal/common.test.hpp>


#include <iostream>


namespace {


struct uri_scheme
  : public sal_test::fixture
{
  static void SetUpTestCase ()
  {
    static const sal::uri::scheme_t test_scheme
    {
      // .default_port =
      42,

      // .default_path =
      "/Path",

      // .case_insensitive_path =
      true,
    };
    sal::uri::uri_t::register_scheme("test", test_scheme);
  }
};


TEST_F(uri_scheme, test_scheme)
{
  auto uri = sal::uri::make_uri("test://h");
  EXPECT_EQ("test", uri.view().scheme);
  EXPECT_EQ("/Path", uri.view().path);
  EXPECT_EQ(42, uri.port());
  EXPECT_TRUE(uri.scheme().case_insensitive_path);
}


TEST_F(uri_scheme, generic_scheme)
{
  auto uri = sal::uri::make_uri("any://h");
  EXPECT_EQ("any", uri.view().scheme);
  EXPECT_EQ("", uri.view().path);
  EXPECT_EQ(0, uri.port());
  EXPECT_FALSE(uri.scheme().case_insensitive_path);
}


TEST_F(uri_scheme, mailto_scheme)
{
  auto uri = sal::uri::make_uri("MailTo:h");
  EXPECT_EQ("mailto", uri.view().scheme);
  EXPECT_EQ("h", uri.view().path);
  EXPECT_EQ(0, uri.port());
  EXPECT_TRUE(uri.scheme().case_insensitive_path);
}


TEST_F(uri_scheme, ftp_scheme)
{
  auto uri = sal::uri::make_uri("Ftp://h");
  EXPECT_EQ("ftp", uri.view().scheme);
  EXPECT_EQ("/", uri.view().path);
  EXPECT_EQ(21, uri.port());
  EXPECT_FALSE(uri.scheme().case_insensitive_path);
}


TEST_F(uri_scheme, http_scheme)
{
  auto uri = sal::uri::make_uri("Http://h");
  EXPECT_EQ("http", uri.view().scheme);
  EXPECT_EQ("/", uri.view().path);
  EXPECT_EQ(80, uri.port());
  EXPECT_FALSE(uri.scheme().case_insensitive_path);
}


TEST_F(uri_scheme, https_scheme)
{
  auto uri = sal::uri::make_uri("HttpS://h");
  EXPECT_EQ("https", uri.view().scheme);
  EXPECT_EQ("/", uri.view().path);
  EXPECT_EQ(443, uri.port());
  EXPECT_FALSE(uri.scheme().case_insensitive_path);
}


} // namespace
