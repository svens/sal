#include <sal/uri.hpp>
#include <sal/common.test.hpp>
#include <ostream>


namespace sal {


std::ostream &print (std::ostream &os, const std::string_view &piece)
{
  if (!piece.data())
  {
    return (os << "{}");
  }
  else if (piece.empty())
  {
    return (os << "''");
  }
  return (os << piece);
}


std::ostream &operator<< (std::ostream &os, const sal::uri_view_t &uri)
{
  print(os, uri.scheme) << '|';
  print(os, uri.user_info) << '|';
  print(os, uri.host) << '|';
  print(os, uri.port) << '|';
  print(os, uri.path) << '|';
  print(os, uri.query) << '|';
  print(os, uri.fragment);
  return os;
}


bool cmp (const std::string_view &left, const std::string_view &right) noexcept
{
  if (left.data() && right.data())
  {
    return left == right;
  }
  else if (!left.data() && !right.data())
  {
    return true;
  }
  return false;
}


bool operator== (const uri_view_t &left, const uri_view_t &right) noexcept
{
  return cmp(left.scheme, right.scheme)
    && cmp(left.user_info, right.user_info)
    && cmp(left.host, right.host)
    && cmp(left.port, right.port)
    && cmp(left.path, right.path)
    && cmp(left.query, right.query)
    && cmp(left.fragment, right.fragment)
  ;
}


} // namespace sal


namespace {


struct test_case_t
{
  std::string uri{};

  union
  {
    sal::uri_view_t expected_components{};
    std::error_code expected_error;
  };
  bool expect_success;

  test_case_t (const sal::uri_view_t &expected)
    : expected_components(expected)
    , expect_success(true)
  {}

  test_case_t (sal::uri_errc expected)
    : expected_error(sal::make_error_code(expected))
    , expect_success(false)
  {}

  friend std::ostream &operator<< (std::ostream &os, const test_case_t &test_case)
  {
    return (os << '\'' << test_case.uri << '\'');
  }
};


test_case_t ok (std::string uri, const sal::uri_view_t &components)
{
  test_case_t result{components};
  result.uri = uri;
  return result;
}


test_case_t fail (std::string uri, sal::uri_errc error_code)
{
  test_case_t result{error_code};
  result.uri = uri;
  return result;
}


sal::uri_view_t uri (
  const std::string_view &scheme,
  const std::string_view &user_info,
  const std::string_view &host,
  const std::string_view &port,
  const std::string_view &path,
  const std::string_view &query,
  const std::string_view &fragment) noexcept
{
  sal::uri_view_t uri;
  uri.scheme = scheme;
  uri.user_info = user_info;
  uri.host = host;
  uri.port = port;
  uri.path = path;
  uri.query = query;
  uri.fragment = fragment;
  return uri;
}


test_case_t test_cases[] =
{
  ok("scheme://user:pass@host:12345/path?query#fragment",
    uri(
      "scheme",
      "user:pass",
      "host",
      "12345",
      "/path",
      "query",
      "fragment"
    )
  ),

  ok(" \t\r\nscheme://user:pass@host:12345/path?query#fragment\t\r\n ",
    uri(
      "scheme",
      "user:pass",
      "host",
      "12345",
      "/path",
      "query",
      "fragment"
    )
  ),

  //
  // Tests from https://rosettacode.org/wiki/URL_parser
  //

  ok("foo://example.com:8042/over/there?name=ferret#nose",
    uri(
      "foo",
      {},
      "example.com",
      "8042",
      "/over/there",
      "name=ferret",
      "nose"
    )
  ),

  ok("urn:example:animal:ferret:nose",
    uri(
      "urn",
      {},
      {},
      {},
      "example:animal:ferret:nose",
      {},
      {}
    )
  ),

  ok("jdbc:mysql://test_user:ouupppssss@localhost:3306/sakila?profileSQL=true",
    uri(
      "jdbc",
      {},
      {},
      {},
      "mysql://test_user:ouupppssss@localhost:3306/sakila",
      "profileSQL=true",
      {}
    )
  ),

  ok("ftp://ftp.is.co.za/rfc/rfc1808.txt",
    uri(
      "ftp",
      {},
      "ftp.is.co.za",
      {},
      "/rfc/rfc1808.txt",
      {},
      {}
    )
  ),

  ok("http://www.ietf.org/rfc/rfc2396.txt#header1",
    uri(
      "http",
      {},
      "www.ietf.org",
      {},
      "/rfc/rfc2396.txt",
      {},
      "header1"
    )
  ),

  ok("ldap://[2001:db8::7]/c=GB?objectClass=one&objectClass=two",
    uri(
      "ldap",
      {},
      "[2001:db8::7]",
      {},
      "/c=GB",
      "objectClass=one&objectClass=two",
      {}
    )
  ),

  ok("mailto:John.Doe@example.com",
    uri(
      "mailto",
      {},
      {},
      {},
      "John.Doe@example.com",
      {},
      {}
    )
  ),

  ok("news:comp.infosystems.www.servers.unix",
    uri(
      "news",
      {},
      {},
      {},
      "comp.infosystems.www.servers.unix",
      {},
      {}
    )
  ),

  ok("tel:+1-816-555-1212",
    uri(
      "tel",
      {},
      {},
      {},
      "+1-816-555-1212",
      {},
      {}
    )
  ),

  ok("telnet://192.0.2.16:80/",
    uri(
      "telnet",
      {},
      "192.0.2.16",
      "80",
      "/",
      {},
      {}
    )
  ),

  ok("urn:oasis:names:specification:docbook:dtd:xml:4.1.2",
    uri(
      "urn",
      {},
      {},
      {},
      "oasis:names:specification:docbook:dtd:xml:4.1.2",
      {},
      {}
    )
  ),

  ok("ftp://cnn.example.com&story=breaking_news@10.0.0.1/top_story.htm",
    uri(
      "ftp",
      "cnn.example.com&story=breaking_news",
      "10.0.0.1",
      {},
      "/top_story.htm",
      {},
      {}
    )
  ),

  //
  // Systematic tests
  // For all combinations, it would be over 6k cases. Use only some
  //

  // uri                            scheme	user		host		port		path		query		fragment
  ok({},			uri({},		{},		{},		{},		{},		{},		{}	)),
  ok("\t\r\n",			uri({},		{},		{},		{},		{},		{},		{}	)),
  ok("\t\r\n\0\t",		uri({},		{},		{},		{},		{},		{},		{}	)),

  ok("#f",			uri({},		{},		{},		{},		{},		{},		"f"	)),
  ok("?q",			uri({},		{},		{},		{},		{},		"q",		{}	)),
  ok("?#",			uri({},		{},		{},		{},		{},		"",		""	)),
  ok("?#f",			uri({},		{},		{},		{},		{},		"",		"f"	)),
  ok("?q#",			uri({},		{},		{},		{},		{},		"q",		""	)),
  ok("?q#f",			uri({},		{},		{},		{},		{},		"q",		"f"	)),

  ok("/",			uri({},		{},		{},		{},		"/",		{},		{}	)),
  ok("/p",			uri({},		{},		{},		{},		"/p",		{},		{}	)),
  ok("./p",			uri({},		{},		{},		{},		"./p",		{},		{}	)),
  ok("../p",			uri({},		{},		{},		{},		"../p",		{},		{}	)),

  ok("//h",			uri({},		{},		"h",		{},		{},		{},		{}	)),
  ok("//h/",			uri({},		{},		"h",		{},		"/",		{},		{}	)),
  ok("//h/p",			uri({},		{},		"h",		{},		"/p",		{},		{}	)),
  ok("//h./p",			uri({},		{},		"h.",		{},		"/p",		{},		{}	)),
  ok("//h../p",			uri({},		{},		"h..",		{},		"/p",		{},		{}	)),

  ok("//u:p@h",			uri({},		"u:p",		"h",		{},		{},		{},		{}	)),
  ok("//u:@h",			uri({},		"u:",		"h",		{},		{},		{},		{}	)),
  ok("//:p@h",			uri({},		":p",		"h",		{},		{},		{},		{}	)),
  ok("//:@h",			uri({},		":",		"h",		{},		{},		{},		{}	)),
  ok("//@h",			uri({},		"",		"h",		{},		{},		{},		{}	)),
  ok("//@",			uri({},		"",		"",		{},		{},		{},		{}	)),
  ok("//@/",			uri({},		"",		"",		{},		"/",		{},		{}	)),

  ok("//h:123",			uri({},		{},		"h",		"123",		{},		{},		{}	)),
  ok("//:123",			uri({},		{},		"",		"123",		{},		{},		{}	)),
  ok("//h:",			uri({},		{},		"h",		"",		{},		{},		{}	)),
  ok("//10.0.0.1:123",		uri({},		{},		"10.0.0.1",	"123",		{},		{},		{}	)),
  ok("//10.0.0.1:",		uri({},		{},		"10.0.0.1",	"",		{},		{},		{}	)),
  ok("//[::1]:123",		uri({},		{},		"[::1]",	"123",		{},		{},		{}	)),
  ok("//[::1]:",		uri({},		{},		"[::1]",	"",		{},		{},		{}	)),

  ok("s://h",			uri("s",	{},		"h",		{},		{},		{},		{}	)),
  ok("s://h/",			uri("s",	{},		"h",		{},		"/",		{},		{}	)),
  ok("s://h/p",			uri("s",	{},		"h",		{},		"/p",		{},		{}	)),
  ok("s://h./p",		uri("s",	{},		"h.",		{},		"/p",		{},		{}	)),
  ok("s://h../p",		uri("s",	{},		"h..",		{},		"/p",		{},		{}	)),

  ok("s://u:p@h",		uri("s",	"u:p",		"h",		{},		{},		{},		{}	)),
  ok("s://u:@h",		uri("s",	"u:",		"h",		{},		{},		{},		{}	)),
  ok("s://:p@h",		uri("s",	":p",		"h",		{},		{},		{},		{}	)),
  ok("s://:@h",			uri("s",	":",		"h",		{},		{},		{},		{}	)),
  ok("s://@h",			uri("s",	"",		"h",		{},		{},		{},		{}	)),
  ok("s://@",			uri("s",	"",		"",		{},		{},		{},		{}	)),
  ok("s://@/",			uri("s",	"",		"",		{},		"/",		{},		{}	)),

  ok("s://h:123",		uri("s",	{},		"h",		"123",		{},		{},		{}	)),
  ok("s://:123",		uri("s",	{},		"",		"123",		{},		{},		{}	)),
  ok("s://h:",			uri("s",	{},		"h",		"",		{},		{},		{}	)),
  ok("s://10.0.0.1:123",	uri("s",	{},		"10.0.0.1",	"123",		{},		{},		{}	)),
  ok("s://10.0.0.1:",		uri("s",	{},		"10.0.0.1",	"",		{},		{},		{}	)),
  ok("s://[::1]:123",		uri("s",	{},		"[::1]",	"123",		{},		{},		{}	)),
  ok("s://[::1]:",		uri("s",	{},		"[::1]",	"",		{},		{},		{}	)),

  ok("s:",			uri("s",	{},		{},		{},		{},		{},		{}	)),
  ok("s:p",			uri("s",	{},		{},		{},		"p",		{},		{}	)),
  ok("s:/",			uri("s",	{},		{},		{},		"/",		{},		{}	)),
  ok("s:/p",			uri("s",	{},		{},		{},		"/p",		{},		{}	)),
  ok("s://",			uri("s",	{},		{},		{},		{},		{},		{}	)),
  ok("s:///",			uri("s",	{},		{},		{},		"/",		{},		{}	)),
  ok("s:///p",			uri("s",	{},		{},		{},		"/p",		{},		{}	)),
  ok("s://./p",			uri("s",	{},		".",		{},		"/p",		{},		{}	)),
  ok("s://../p",		uri("s",	{},		"..",		{},		"/p",		{},		{}	)),
  ok("s:///./p",		uri("s",	{},		{},		{},		"/./p",		{},		{}	)),
  ok("s:///../p",		uri("s",	{},		{},		{},		"/../p",	{},		{}	)),

  fail("1s:", sal::uri_errc::invalid_scheme),
  fail(":", sal::uri_errc::invalid_scheme),
  fail(":/", sal::uri_errc::invalid_scheme),
  fail("://", sal::uri_errc::invalid_scheme),
  fail(":///", sal::uri_errc::invalid_scheme),
  fail(":///p", sal::uri_errc::invalid_scheme),
  fail("://h", sal::uri_errc::invalid_scheme),
  fail("://h:123", sal::uri_errc::invalid_scheme),
  fail(":123", sal::uri_errc::invalid_scheme),
  fail(":123/", sal::uri_errc::invalid_scheme),
  fail(":123//", sal::uri_errc::invalid_scheme),
  fail(":123//path", sal::uri_errc::invalid_scheme),
  fail("s~e:", sal::uri_errc::invalid_scheme),
  fail("s://h|t", sal::uri_errc::invalid_authority),
  fail("s://h/|p", sal::uri_errc::invalid_path),
  fail("s://h/p?<q", sal::uri_errc::invalid_query),
  fail("s://h/p#<p", sal::uri_errc::invalid_fragment),
};

using uri_view = ::testing::TestWithParam<test_case_t>;
INSTANTIATE_TEST_CASE_P(uri, uri_view, ::testing::ValuesIn(test_cases),);


TEST_P(uri_view, split)
{
  auto &test = GetParam();

  std::error_code error;
  auto view = sal::uri_view(test.uri, error);

  if (test.expect_success)
  {
    EXPECT_TRUE(!error);
    EXPECT_EQ(test.expected_components, view);
  }
  else
  {
    EXPECT_EQ(test.expected_error, error) << error.message();
    EXPECT_FALSE(error.message().empty());
    EXPECT_STREQ("uri", error.category().name());

    EXPECT_THROW(sal::uri_view(test.uri), std::system_error);
  }
}


} // namespace
