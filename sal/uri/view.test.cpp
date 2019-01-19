#include <sal/uri/error.hpp>
#include <sal/uri/view.hpp>
#include <sal/common.test.hpp>
#include <ostream>


__sal_begin

namespace uri {

// sal::uri library does not provide inserter or comparison operators,
// implement here for testing

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

std::ostream &operator<< (std::ostream &os, const view_t &uri)
{
  print(os, uri.scheme) << '|';
  print(os, uri.authority) << '|';
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
  return !left.data() && !right.data();
}

bool operator== (const view_t &left, const view_t &right) noexcept
{
  return left.has_scheme() == right.has_scheme()
    && cmp(left.scheme, right.scheme)

    && left.has_authority() == right.has_authority()
    && cmp(left.authority, right.authority)

    && left.has_user_info() == right.has_user_info()
    && cmp(left.user_info, right.user_info)

    && left.has_host() == right.has_host()
    && cmp(left.host, right.host)

    && left.has_port() == right.has_port()
    && cmp(left.port, right.port)

    && left.has_path() == right.has_path()
    && cmp(left.path, right.path)

    && left.has_query() == right.has_query()
    && cmp(left.query, right.query)

    && left.has_fragment() == right.has_fragment()
    && cmp(left.fragment, right.fragment)
  ;
}

} // namespace uri

__sal_end


namespace {


struct view_ok_t //{{{1
{
  std::string input;
  sal::uri::view_t expected_result{};

  view_ok_t (std::string input,
      const std::string_view &scheme,
      const std::string_view &authority,
      const std::string_view &user_info,
      const std::string_view &host,
      const std::string_view &port,
      const std::string_view &path,
      const std::string_view &query,
      const std::string_view &fragment)
    : input(input)
  {
    expected_result.scheme = scheme;
    expected_result.authority = authority;
    expected_result.user_info = user_info;
    expected_result.host = host;
    expected_result.port = port;
    expected_result.path = path;
    expected_result.query = query;
    expected_result.fragment = fragment;
  }

  friend inline std::ostream &operator<< (std::ostream &os, const view_ok_t &test)
  {
    return (os << '\'' << test.input << '\'');
  }
};


view_ok_t view_ok_data[] =
{
  {
    "scheme://user:pass@host:12345/path?query#fragment",
    "scheme",
    "user:pass@host:12345",
    "user:pass",
    "host",
    "12345",
    "/path",
    "query",
    "fragment",
  },

  {
    " \t\r\nscheme://user:pass@host:12345/path?query#fragment\t\r\n ",
    "scheme",
    "user:pass@host:12345",
    "user:pass",
    "host",
    "12345",
    "/path",
    "query",
    "fragment",
  },

  //
  // Tests from https://rosettacode.org/wiki/URL_parser
  //

  {
    "foo://example.com:8042/over/there?name=ferret#nose",
    "foo",
    "example.com:8042",
    {},
    "example.com",
    "8042",
    "/over/there",
    "name=ferret",
    "nose",
  },

  {
    "urn:example:animal:ferret:nose",
    "urn",
    {},
    {},
    {},
    {},
    "example:animal:ferret:nose",
    {},
    {},
  },

  {
    "jdbc:mysql://test_user:ouupppssss@localhost:3306/sakila?profileSQL=true",
    "jdbc",
    {},
    {},
    {},
    {},
    "mysql://test_user:ouupppssss@localhost:3306/sakila",
    "profileSQL=true",
    {},
  },

  {
    "ftp://ftp.is.co.za/rfc/rfc1808.txt",
    "ftp",
    "ftp.is.co.za",
    {},
    "ftp.is.co.za",
    {},
    "/rfc/rfc1808.txt",
    {},
    {},
  },

  {
    "http://www.ietf.org/rfc/rfc2396.txt#header1",
    "http",
    "www.ietf.org",
    {},
    "www.ietf.org",
    {},
    "/rfc/rfc2396.txt",
    {},
    "header1",
  },

  {
    "ldap://[2001:db8::7]/c=GB?objectClass=one&objectClass=two",
    "ldap",
    "[2001:db8::7]",
    {},
    "[2001:db8::7]",
    {},
    "/c=GB",
    "objectClass=one&objectClass=two",
    {},
  },

  {
    "mailto:John.Doe@example.com",
    "mailto",
    {},
    {},
    {},
    {},
    "John.Doe@example.com",
    {},
    {},
  },

  {
    "news:comp.infosystems.www.servers.unix",
    "news",
    {},
    {},
    {},
    {},
    "comp.infosystems.www.servers.unix",
    {},
    {},
  },

  {
    "tel:+1-816-555-1212",
    "tel",
    {},
    {},
    {},
    {},
    "+1-816-555-1212",
    {},
    {},
  },

  {
    "telnet://192.0.2.16:80/",
    "telnet",
    "192.0.2.16:80",
    {},
    "192.0.2.16",
    "80",
    "/",
    {},
    {},
  },

  {
    "urn:oasis:names:specification:docbook:dtd:xml:4.1.2",
    "urn",
    {},
    {},
    {},
    {},
    "oasis:names:specification:docbook:dtd:xml:4.1.2",
    {},
    {},
  },

  {
    "ftp://cnn.example.com&story=breaking_news@10.0.0.1/top_story.htm",
    "ftp",
    "cnn.example.com&story=breaking_news@10.0.0.1",
    "cnn.example.com&story=breaking_news",
    "10.0.0.1",
    {},
    "/top_story.htm",
    {},
    {},
  },

  //
  // Systematic tests
  // For all combinations, it would be over 6k cases. Use only some
  //

  // input		scheme	auth	user	host	port	path	query	fragment
  {{},			{},	{},	{},	{},	{},	{},	{},	{}	},
  {"\t\r\n",		{},	{},	{},	{},	{},	{},	{},	{}	},
  {"\t\r\n\0\t",	{},	{},	{},	{},	{},	{},	{},	{}	},

  {"#f",		{},	{},	{},	{},	{},	{},	{},	"f"	},
  {"?q",		{},	{},	{},	{},	{},	{},	"q",	{}	},
  {"?#",		{},	{},	{},	{},	{},	{},	"",	""	},
  {"?#f",		{},	{},	{},	{},	{},	{},	"",	"f"	},
  {"?q#",		{},	{},	{},	{},	{},	{},	"q",	""	},
  {"?q#f",		{},	{},	{},	{},	{},	{},	"q",	"f"	},

  {"/",			{},	{},	{},	{},	{},	"/",	{},	{}	},
  {"/p",		{},	{},	{},	{},	{},	"/p",	{},	{}	},
  {"./p",		{},	{},	{},	{},	{},	"./p",	{},	{}	},
  {"../p",		{},	{},	{},	{},	{},	"../p",	{},	{}	},

  {"//h",		{},	"h",	{},	"h",	{},	{},	{},	{}	},
  {"//h/",		{},	"h",	{},	"h",	{},	"/",	{},	{}	},
  {"//h/p",		{},	"h",	{},	"h",	{},	"/p",	{},	{}	},
  {"//h./p",		{},	"h.",	{},	"h.",	{},	"/p",	{},	{}	},
  {"//h../p",		{},	"h..",	{},	"h..",	{},	"/p",	{},	{}	},

  {"//u:p@h",		{},	"u:p@h",	"u:p",	"h",	{},	{},	{},	{}	},
  {"//u:@h",		{},	"u:@h",	"u:",	"h",	{},	{},	{},	{}	},
  {"//:p@h",		{},	":p@h",	":p",	"h",	{},	{},	{},	{}	},
  {"//:@h",		{},	":@h",	":",	"h",	{},	{},	{},	{}	},
  {"//@h",		{},	"@h",	"",	"h",	{},	{},	{},	{}	},
  {"//@",		{},	"@",	"",	"",	{},	{},	{},	{}	},
  {"//@/",		{},	"@",	"",	"",	{},	"/",	{},	{}	},

  {"//h:123",		{},	"h:123",	{},	"h",	"123",	{},	{},	{}	},
  {"//:123",		{},	":123",	{},	"",	"123",	{},	{},	{}	},
  {"//h:-123",		{},	"h:-123",	{},	"h:-123",	{},	{},	{},	{}	},
  {"//h:",		{},	"h:",	{},	"h",	"",	{},	{},	{}	},
  {"//10.0.0.1:123",	{},	"10.0.0.1:123",	{},	"10.0.0.1",	"123",{},	{},	{}	},
  {"//10.0.0.1:",	{},	"10.0.0.1:",	{},	"10.0.0.1",	"",	{},	{},	{}	},
  {"//[::1]:123",	{},	"[::1]:123",	{},	"[::1]",	"123",	{},	{},	{}	},
  {"//[::1]:",		{},	"[::1]:",	{},	"[::1]",	"",	{},	{},	{}	},
  {"//123",		{},	"123",	{},	"123",	{},	{},	{},	{}	},

  {"s://h",		"s",	"h",	{},	"h",	{},	{},	{},	{}	},
  {"s://h/",		"s",	"h",	{},	"h",	{},	"/",	{},	{}	},
  {"s://h/p",		"s",	"h",	{},	"h",	{},	"/p",	{},	{}	},
  {"s://h./p",		"s",	"h.",	{},	"h.",	{},	"/p",	{},	{}	},
  {"s://h../p",		"s",	"h..",	{},	"h..",	{},	"/p",	{},	{}	},

  {"s://u:p@h",		"s",	"u:p@h",	"u:p",	"h",	{},	{},	{},	{}	},
  {"s://u:@h",		"s",	"u:@h",	"u:",	"h",	{},	{},	{},	{}	},
  {"s://:p@h",		"s",	":p@h",	":p",	"h",	{},	{},	{},	{}	},
  {"s://:@h",		"s",	":@h",	":",	"h",	{},	{},	{},	{}	},
  {"s://@h",		"s",	"@h",	"",	"h",	{},	{},	{},	{}	},
  {"s://@",		"s",	"@",	"",	"",	{},	{},	{},	{}	},
  {"s://@/",		"s",	"@",	"",	"",	{},	"/",	{},	{}	},

  {"s://h:123",		"s",	"h:123",	{},	"h",		"123",	{},	{},	{}	},
  {"s://:123",		"s",	":123",	{},	"",		"123",	{},	{},	{}	},
  {"s://h:",		"s",	"h:",	{},	"h",		"",	{},	{},	{}	},
  {"s://10.0.0.1:123",	"s",	"10.0.0.1:123",	{},	"10.0.0.1",	"123",	{},	{},	{}	},
  {"s://10.0.0.1:",	"s",	"10.0.0.1:",	{},	"10.0.0.1",	"",	{},	{},	{}	},
  {"s://[::1]:123",	"s",	"[::1]:123",	{},	"[::1]",	"123",	{},	{},	{}	},
  {"s://[::1]:",	"s",	"[::1]:",	{},	"[::1]",	"",	{},	{},	{}	},

  {"s:",		"s",	{},	{},	{},	{},	{},	{},	{}	},
  {"s:p",		"s",	{},	{},	{},	{},	"p",	{},	{}	},
  {"s:/",		"s",	{},	{},	{},	{},	"/",	{},	{}	},
  {"s:/p",		"s",	{},	{},	{},	{},	"/p",	{},	{}	},
  {"s://",		"s",	"",	{},	{},	{},	{},	{},	{}	},
  {"s:///",		"s",	"",	{},	{},	{},	"/",	{},	{}	},
  {"s:///p",		"s",	"",	{},	{},	{},	"/p",	{},	{}	},
  {"s://./p",		"s",	".",	{},	".",	{},	"/p",	{},	{}	},
  {"s://../p",		"s",	"..",	{},	"..",	{},	"/p",	{},	{}	},
  {"s:///./p",		"s",	"",	{},	{},	{},	"/./p",	{},	{}	},
  {"s:///../p",		"s",	"",	{},	{},	{},	"/../p",	{},	{}	},

  {"s://\x80@h/p?q#f",	"s",	"\x80@h",	"\x80",	"h",	{},	"/p",	"q",	"f"	},
  {"s://u@\x80/p?q#f",	"s",	"u@\x80",	"u",	"\x80",	{},	"/p",	"q",	"f"	},
  {"s://u@h/\x80?q#f",	"s",	"u@h",	"u",	"h",	{},	"/\x80",	"q",	"f"	},
  {"s://u@h/p?\x80#f",	"s",	"u@h",	"u",	"h",	{},	"/p",	"\x80",	"f"	},
  {"s://u@h/p?q#\x80",	"s",	"u@h",	"u",	"h",	{},	"/p",	"q",	"\x80"	},
};


using view_ok = ::testing::TestWithParam<view_ok_t>;
INSTANTIATE_TEST_CASE_P(uri, view_ok, ::testing::ValuesIn(view_ok_data),);


TEST_P(view_ok, test)
{
  auto &test = GetParam();
  EXPECT_EQ(test.expected_result, sal::uri::make_view(test.input));

  std::error_code error;
  EXPECT_EQ(test.expected_result,
    sal::uri::make_view(test.input.begin(), test.input.end())
  );
  EXPECT_FALSE(error);

  EXPECT_EQ(test.expected_result,
    sal::uri::make_view(test.input.begin(), test.input.end(), error)
  );
  EXPECT_FALSE(error);

  EXPECT_NO_THROW(
    EXPECT_EQ(test.expected_result,
      sal::uri::make_view(test.input.begin(), test.input.end())
    )
  );
}


struct view_fail_t //{{{1
{
  std::string input;
  sal::uri::errc expected_errc;

  friend std::ostream &operator<< (std::ostream &os, const view_fail_t &test)
  {
    return (os << '\'' << test.input << '\'');
  }
};


view_fail_t view_fail_data[] =
{
  { "s\x80://u@h/p?q#f", sal::uri::errc::invalid_scheme },
  { "1s:", sal::uri::errc::invalid_scheme },
  { ":", sal::uri::errc::invalid_scheme },
  { ":/", sal::uri::errc::invalid_scheme },
  { "://", sal::uri::errc::invalid_scheme },
  { ":///", sal::uri::errc::invalid_scheme },
  { ":///p", sal::uri::errc::invalid_scheme },
  { "://h", sal::uri::errc::invalid_scheme },
  { "://h:123", sal::uri::errc::invalid_scheme },
  { ":123", sal::uri::errc::invalid_scheme },
  { ":123/", sal::uri::errc::invalid_scheme },
  { ":123//", sal::uri::errc::invalid_scheme },
  { ":123//path", sal::uri::errc::invalid_scheme },
  { "s~e:", sal::uri::errc::invalid_scheme },
  { "s://h|t", sal::uri::errc::invalid_authority },
  { "s://h/|p", sal::uri::errc::invalid_path },
  { "s://h/p?<q", sal::uri::errc::invalid_query },
  { "s://h/p#<p", sal::uri::errc::invalid_fragment },
};


using view_fail = ::testing::TestWithParam<view_fail_t>;
INSTANTIATE_TEST_CASE_P(uri, view_fail, ::testing::ValuesIn(view_fail_data),);


TEST_P(view_fail, test)
{
  auto &test = GetParam();

  std::error_code error;
  (void)sal::uri::make_view(test.input, error);
  auto expected_error = sal::uri::make_error_code(test.expected_errc);
  EXPECT_EQ(expected_error, error) << error.message();
  EXPECT_FALSE(error.message().empty());
  EXPECT_EQ(sal::uri::category(), error.category());
  EXPECT_STREQ("uri", error.category().name());

  EXPECT_THROW(
    sal::uri::make_view(test.input),
    std::system_error
  );

  EXPECT_THROW(
    sal::uri::make_view(test.input.begin(), test.input.end()),
    std::system_error
  );

  (void)sal::uri::make_view(test.input.begin(), test.input.end(), error);
  EXPECT_EQ(expected_error, error);
}


//}}}1


} // namespace
