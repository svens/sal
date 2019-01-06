#include <sal/uri/uri.hpp>
#include <sal/common.test.hpp>


namespace {


struct ctor_ok_t
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


ctor_ok_t success_test[] =
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
    "John.Doe@example.com",
    {},
    {},
    "mailto:John.Doe@example.com",
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

#if 0
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
#endif

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
};


using uri_ctor = ::testing::TestWithParam<ctor_ok_t>;
INSTANTIATE_TEST_CASE_P(uri, uri_ctor, ::testing::ValuesIn(success_test),);


TEST_P(uri_ctor, test)
{
  const auto &test = GetParam();
  auto uri = sal::uri::make_uri(test.input);
  EXPECT_EQ(
    std::string{test.expected_scheme},
    std::string{uri.to_view().scheme}
  );
  EXPECT_EQ(
    std::string{test.expected_user_info},
    std::string{uri.to_view().user_info}
  );
  EXPECT_EQ(
    std::string{test.expected_host},
    std::string{uri.to_view().host}
  );
  EXPECT_EQ(
    std::string{test.expected_port},
    std::string{uri.to_view().port}
  );
  EXPECT_EQ(
    std::string{test.expected_path},
    std::string{uri.to_view().path}
  );
  EXPECT_EQ(
    std::string{test.expected_query},
    std::string{uri.to_view().query}
  );
  EXPECT_EQ(
    std::string{test.expected_fragment},
    std::string{uri.to_view().fragment}
  );
  EXPECT_EQ(
    test.expected_encoded_string,
    uri.to_encoded_string()
  );
}


} // namespace
