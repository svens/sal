#include <sal/uri/uri.hpp>
#include <sal/common.test.hpp>


namespace {


struct ctor_ok_t //{{{1
{
  std::string input;

  std::string_view
    expected_scheme,
    expected_user_info,
    expected_host,
    expected_port,
    expected_path,
    expected_query,
    expected_fragment;

  std::string expected_encoded_string;

  friend std::ostream &operator<< (std::ostream &os, const ctor_ok_t &test)
  {
    return (os << '\'' << test.input << '\'');
  }
};


ctor_ok_t ctor_ok[] =
{
  {
    "scheme://user%40info@host:12345/path?query#fragment",
    "scheme",
    "user@info",
    "host",
    "12345",
    "/path",
    "query",
    "fragment",
    "scheme://user%40info@host:12345/path?query#fragment",
  },

  {
    "foo://example.com:8042/over/there?name=ferret#nose",
    "foo",
    {},
    "example.com",
    "8042",
    "/over/there",
    "name=ferret",
    "nose",
    "foo://example.com:8042/over/there?name=ferret#nose",
  },

  {
    "urn:example:animal:ferret:nose",
    "urn",
    {},
    {},
    {},
    "example:animal:ferret:nose",
    {},
    {},
    "urn:example:animal:ferret:nose",
  },

  {
    "jdbc:mysql://test_user:ouupppssss@localhost:3306/sakila?profileSQL=true",
    "jdbc",
    {},
    {},
    {},
    "mysql://test_user:ouupppssss@localhost:3306/sakila",
    "profileSQL=true",
    {},
    "jdbc:mysql://test_user:ouupppssss@localhost:3306/sakila?profileSQL=true",
  },

  {
    "ftp://ftp.is.co.za/rfc/rfc1808.txt",
    "ftp",
    {},
    "ftp.is.co.za",
    {},
    "/rfc/rfc1808.txt",
    {},
    {},
    "ftp://ftp.is.co.za/rfc/rfc1808.txt",
  },

  {
    "http://www.ietf.org/rfc/rfc2396.txt#header1",
    "http",
    {},
    "www.ietf.org",
    {},
    "/rfc/rfc2396.txt",
    {},
    "header1",
    "http://www.ietf.org/rfc/rfc2396.txt#header1",
  },

  {
    "ldap://[2001:db8::7]/c=GB?objectClass=one&objectClass=two",
    "ldap",
    {},
    "[2001:db8::7]",
    {},
    "/c=GB",
    "objectClass=one&objectClass=two",
    {},
    "ldap://[2001:db8::7]/c=GB?objectClass=one&objectClass=two",
  },

  {
    "mailto:John.Doe@example.com",
    "mailto",
    {},
    {},
    {},
    "john.doe@example.com",
    {},
    {},
    "mailto:john.doe@example.com",
  },

  {
    "mailto:John.Doe@example.com?subject=Test",
    "mailto",
    {},
    {},
    {},
    "john.doe@example.com",
    "subject=Test",
    {},
    "mailto:john.doe@example.com?subject=Test",
  },

  {
    "news:comp.infosystems.www.servers.unix",
    "news",
    {},
    {},
    {},
    "comp.infosystems.www.servers.unix",
    {},
    {},
    "news:comp.infosystems.www.servers.unix",
  },

  {
    "tel:+1-816-555-1212",
    "tel",
    {},
    {},
    {},
    "+1-816-555-1212",
    {},
    {},
    "tel:+1-816-555-1212",
  },

  {
    "telnet://192.0.2.16:80/",
    "telnet",
    {},
    "192.0.2.16",
    "80",
    "/",
    {},
    {},
    "telnet://192.0.2.16:80/",
  },

  {
    "tcp://:80",
    "tcp",
    {},
    {},
    "80",
    {},
    {},
    {},
    "tcp://:80",
  },

  {
    "urn:oasis:names:specification:docbook:dtd:xml:4.1.2",
    "urn",
    {},
    {},
    {},
    "oasis:names:specification:docbook:dtd:xml:4.1.2",
    {},
    {},
    "urn:oasis:names:specification:docbook:dtd:xml:4.1.2",
  },

  {
    "ftp://cnn.example.com&story=breaking_news@10.0.0.1/top_story.htm",
    "ftp",
    "cnn.example.com&story=breaking_news",
    "10.0.0.1",
    {},
    "/top_story.htm",
    {},
    {},
    "ftp://cnn.example.com&story=breaking_news@10.0.0.1/top_story.htm",
  },

  {
    "eXAMPLE://a/./b/../b/%63/%7bfoo%7d",
    "example",
    {},
    "a",
    {},
    "/b/c/{foo}",
    {},
    {},
    "example://a/b/c/%7Bfoo%7D",
  },

  {
    "HTTP://www.EXAMPLE.com/",
    "http",
    {},
    "www.example.com",
    {},
    "/",
    {},
    {},
    "http://www.example.com/",
  },

  {
    "http://example.com/",
    "http",
    {},
    "example.com",
    {},
    "/",
    {},
    {},
    "http://example.com/",
  },

  {
    "http://example.com",
    "http",
    {},
    "example.com",
    {},
    "/",
    {},
    {},
    "http://example.com/",
  },

  {
    "http://example.com:/",
    "http",
    {},
    "example.com",
    {},
    "/",
    {},
    {},
    "http://example.com/",
  },

  {
    "http://example.com:80/",
    "http",
    {},
    "example.com",
    {},
    "/",
    {},
    {},
    "http://example.com/",
  },

  {
    "https://example.com:443/",
    "https",
    {},
    "example.com",
    {},
    "/",
    {},
    {},
    "https://example.com/",
  },

  {
    "https://example.com:80/",
    "https",
    {},
    "example.com",
    "80",
    "/",
    {},
    {},
    "https://example.com:80/",
  },

  {
    "/a/b/c/./../../g",
    {},
    {},
    {},
    {},
    "/a/g",
    {},
    {},
    "/a/g"
  },

  {
    "mid/content=5/../6",
    {},
    {},
    {},
    {},
    "mid/6",
    {},
    {},
    "mid/6",
  },

  {
    "http://www%2Eexample%2Ecom/%7E%66%6F%6F%62%61%72%5F%36%39/",
    "http",
    {},
    "www.example.com",
    {},
    "/~foobar_69/",
    {},
    {},
    "http://www.example.com/~foobar_69/",
  },

  {
    "http://www%2eexample%2ecom/%7e%66%6f%6f%62%61%72%5f%36%39/",
    "http",
    {},
    "www.example.com",
    {},
    "/~foobar_69/",
    {},
    {},
    "http://www.example.com/~foobar_69/",
  },

  {
    "http://www.example.com/a/./b/",
    "http",
    {},
    "www.example.com",
    {},
    "/a/b/",
    {},
    {},
    "http://www.example.com/a/b/",
  },

  {
    "http://www.example.com/a/../b/",
    "http",
    {},
    "www.example.com",
    {},
    "/b/",
    {},
    {},
    "http://www.example.com/b/",
  },

  {
    "http://www.example.com/%61/b/",
    "http",
    {},
    "www.example.com",
    {},
    "/a/b/",
    {},
    {},
    "http://www.example.com/a/b/",
  },

  {
    "http://www.example.com/a/%2e%2E/b/",
    "http",
    {},
    "www.example.com",
    {},
    "/b/",
    {},
    {},
    "http://www.example.com/b/",
  },

  {
    "http://www.example.com/a/../b/?key=value",
    "http",
    {},
    "www.example.com",
    {},
    "/b/",
    "key=value",
    {},
    "http://www.example.com/b/?key=value",
  },

  {
    "http://www.example.com/a/../b/#fragment",
    "http",
    {},
    "www.example.com",
    {},
    "/b/",
    {},
    "fragment",
    "http://www.example.com/b/#fragment",
  },

  {
    "http://www.example.com/..",
    "http",
    {},
    "www.example.com",
    {},
    "/",
    {},
    {},
    "http://www.example.com/",
  },

  {
    "http://www.example.com/../..",
    "http",
    {},
    "www.example.com",
    {},
    "/",
    {},
    {},
    "http://www.example.com/",
  },

  {
    "http://www.example.com/a/../..",
    "http",
    {},
    "www.example.com",
    {},
    "/",
    {},
    {},
    "http://www.example.com/",
  },

  {
    "http://www.example.com/.",
    "http",
    {},
    "www.example.com",
    {},
    "/",
    {},
    {},
    "http://www.example.com/",
  },

  {
    "http://www.example.com/./.",
    "http",
    {},
    "www.example.com",
    {},
    "/",
    {},
    {},
    "http://www.example.com/",
  },

  {
    "http://www.example.com/a/./.",
    "http",
    {},
    "www.example.com",
    {},
    "/a/",
    {},
    {},
    "http://www.example.com/a/",
  },

  // TODO: this is not valid as per https://tools.ietf.org/html/rfc3986#section-3.3
  {
    "http://www.example.com/%3a%2f%3f%23%5b%5d%40%21%24%26%27%28%29%2a%2b%2c%3b%3d",
    "http",
    {},
    "www.example.com",
    {},
    "/:/?#[]@!$&'()*+,;=",
    {},
    {},
    "http://www.example.com/:/%3F%23%5B%5D@!$&'()*+,;=",
  },

  {
    "http://www.example.com/a%2d%2e%5f%7e",
    "http",
    {},
    "www.example.com",
    {},
    "/a-._~",
    {},
    {},
    "http://www.example.com/a-._~",
  },

  {
    "http://www.example.com?query=%2d%2e%5f%7e",
    "http",
    {},
    "www.example.com",
    {},
    "/",
    "query=-._~",
    {},
    "http://www.example.com/?query=-._~",
  },

  {
    "http://www.example.com/a//b/",
    "http",
    {},
    "www.example.com",
    {},
    "/a//b/",
    {},
    {},
    "http://www.example.com/a//b/",
  },

  {
    "http://www.example.com/a//..//b/",
    "http",
    {},
    "www.example.com",
    {},
    "/a//b/",
    {},
    {},
    "http://www.example.com/a//b/",
  },

  {
    "/",
    {},
    {},
    {},
    {},
    "/",
    {},
    {},
    "/",
  },

  {
    "/./",
    {},
    {},
    {},
    {},
    "/",
    {},
    {},
    "/",
  },

  {
    "/../",
    {},
    {},
    {},
    {},
    "/",
    {},
    {},
    "/",
  },

  {
    "./",
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
  },

  {
    "../",
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
  },

  {
    "/.",
    {},
    {},
    {},
    {},
    "/",
    {},
    {},
    "/",
  },

  {
    "/..",
    {},
    {},
    {},
    {},
    "/",
    {},
    {},
    "/",
  },

  {
    ".",
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
  },

  {
    "..",
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
  },
};


using uri_ctor_ok = ::testing::TestWithParam<ctor_ok_t>;
INSTANTIATE_TEST_CASE_P(uri, uri_ctor_ok, ::testing::ValuesIn(ctor_ok),);


TEST_P(uri_ctor_ok, test)
{
  const auto &test = GetParam();
  auto uri = sal::uri::make_uri(test.input);
  EXPECT_EQ(
    std::string{test.expected_scheme},
    std::string{uri.view().scheme}
  );
  EXPECT_EQ(
    std::string{test.expected_user_info},
    std::string{uri.view().user_info}
  );
  EXPECT_EQ(
    std::string{test.expected_host},
    std::string{uri.view().host}
  );
  EXPECT_EQ(
    std::string{test.expected_port},
    std::string{uri.view().port}
  );
  EXPECT_EQ(
    std::string{test.expected_path},
    std::string{uri.view().path}
  );
  EXPECT_EQ(
    std::string{test.expected_query},
    std::string{uri.view().query}
  );
  EXPECT_EQ(
    std::string{test.expected_fragment},
    std::string{uri.view().fragment}
  );
  EXPECT_EQ(
    test.expected_encoded_string,
    uri.encoded_string()
  );

  EXPECT_NO_THROW(
    sal::uri::make_uri(test.input.begin(), test.input.end())
  );

  std::error_code error;
  (void)sal::uri::make_uri(test.input.begin(), test.input.end(), error);
  EXPECT_FALSE(error);

  (void)sal::uri::make_uri(test.input, error);
  EXPECT_FALSE(error);
}


struct ctor_fail_t //{{{1
{
  std::string input;
  std::error_code expected_error;

  friend std::ostream &operator<< (std::ostream &os, const ctor_fail_t &test)
  {
    return (os << '\'' << test.input << '\'');
  }
};


ctor_fail_t ctor_fail[] =
{
  { "http://example.com:65536/path", std::make_error_code(std::errc::result_out_of_range) },
  { "s\x80://u@h/p?q#f", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { "1s:", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { ":", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { ":/", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { "://", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { ":///", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { ":///p", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { "://h", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { "://h:123", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { ":123", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { ":123/", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { ":123//", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { ":123//path", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { "s~e:", sal::uri::make_error_code(sal::uri::errc::invalid_scheme) },
  { "s://h|t", sal::uri::make_error_code(sal::uri::errc::invalid_authority) },
  { "s://h/|p", sal::uri::make_error_code(sal::uri::errc::invalid_path) },
  { "s://h/p?<q", sal::uri::make_error_code(sal::uri::errc::invalid_query) },
  { "s://h/p#<p", sal::uri::make_error_code(sal::uri::errc::invalid_fragment) },
};


using uri_ctor_fail = ::testing::TestWithParam<ctor_fail_t>;
INSTANTIATE_TEST_CASE_P(uri, uri_ctor_fail, ::testing::ValuesIn(ctor_fail),);


TEST_P(uri_ctor_fail, test)
{
  const auto &test = GetParam();

  std::error_code error;
  auto uri = sal::uri::make_uri(test.input, error);
  EXPECT_EQ(test.expected_error, error);

  EXPECT_THROW(
    sal::uri::make_uri(test.input),
    std::system_error
  );
}


//}}}1


} // namespace
