#include <sal/uri/error.hpp>
#include <sal/uri/view.hpp>
#include <sal/common.test.hpp>
#include <ostream>


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


TEST_P(view_ok, string)
{
  auto &test = GetParam();
  auto view = sal::uri::make_view(test.input);
  EXPECT_EQ(test.input, view.string());
}


TEST_P(view_ok, inserter)
{
  auto &test = GetParam();
  auto view = sal::uri::make_view(test.input);
  std::ostringstream oss;
  oss << view;
  EXPECT_EQ(test.input, oss.str());
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


// uri_view //{{{1


struct uri_view
  : public sal_test::fixture
{
  const sal::uri::view_t view{"s://u@h:123/p?q#f"};
};


TEST_F(uri_view, empty)
{
  sal::uri::view_t v = view;
  EXPECT_FALSE(v.empty());

  v.scheme = {};
  EXPECT_FALSE(v.empty());

  v.user_info = {};
  EXPECT_FALSE(v.empty());

  v.host = {};
  EXPECT_FALSE(v.empty());

  v.port = {};
  EXPECT_FALSE(v.empty());

  v.path = {};
  EXPECT_FALSE(v.empty());

  v.query = {};
  EXPECT_FALSE(v.empty());

  v.fragment = {};
  EXPECT_TRUE(v.empty());
}


TEST_F(uri_view, authority)
{
  sal::uri::view_t v = view;
  EXPECT_TRUE(v.has_authority());

  v.path = {};
  EXPECT_TRUE(v.has_authority());

  v.query = {};
  EXPECT_TRUE(v.has_authority());

  v.fragment = {};
  EXPECT_TRUE(v.has_authority());

  v.scheme = {};
  EXPECT_TRUE(v.has_authority());

  v.user_info = {};
  EXPECT_TRUE(v.has_authority());

  v.host = {};
  EXPECT_TRUE(v.has_authority());

  v.port = {};
  EXPECT_FALSE(v.has_authority());
}


TEST_F(uri_view, swap)
{
  sal::uri::view_t a = view, b{};

  EXPECT_FALSE(a.empty());
  EXPECT_EQ("s", a.scheme);
  EXPECT_EQ("u@h:123", a.authority);
  EXPECT_EQ("u", a.user_info);
  EXPECT_EQ("h", a.host);
  EXPECT_EQ("123", a.port);
  EXPECT_EQ("/p", a.path);
  EXPECT_EQ("q", a.query);
  EXPECT_EQ("f", a.fragment);

  EXPECT_TRUE(b.empty());

  sal::uri::swap(a, b);

  EXPECT_TRUE(a.empty());

  EXPECT_FALSE(b.empty());
  EXPECT_EQ("s", b.scheme);
  EXPECT_EQ("u@h:123", b.authority);
  EXPECT_EQ("u", b.user_info);
  EXPECT_EQ("h", b.host);
  EXPECT_EQ("123", b.port);
  EXPECT_EQ("/p", b.path);
  EXPECT_EQ("q", b.query);
  EXPECT_EQ("f", b.fragment);
}


TEST_F(uri_view, compare)
{
  sal::uri::view_t v = view;
  EXPECT_EQ(view, v);
  EXPECT_GE(view, v);
  EXPECT_LE(view, v);

  v.fragment = {};
  EXPECT_NE(view, v);
  EXPECT_GE(view, v);
  EXPECT_GT(view, v);
  EXPECT_LE(v, view);
  EXPECT_LT(v, view);

  v.query = {};
  EXPECT_NE(view, v);
  EXPECT_GE(view, v);
  EXPECT_GT(view, v);
  EXPECT_LE(v, view);
  EXPECT_LT(v, view);

  v.path = {};
  EXPECT_NE(view, v);
  EXPECT_GE(view, v);
  EXPECT_GT(view, v);
  EXPECT_LE(v, view);
  EXPECT_LT(v, view);

  v.port = {};
  EXPECT_NE(view, v);
  EXPECT_GE(view, v);
  EXPECT_GT(view, v);
  EXPECT_LE(v, view);
  EXPECT_LT(v, view);

  v.host = {};
  EXPECT_NE(view, v);
  EXPECT_GE(view, v);
  EXPECT_GT(view, v);
  EXPECT_LE(v, view);
  EXPECT_LT(v, view);

  v.user_info = {};
  EXPECT_NE(view, v);
  EXPECT_GE(view, v);
  EXPECT_GT(view, v);
  EXPECT_LE(v, view);
  EXPECT_LT(v, view);

  v.scheme = {};
  EXPECT_NE(view, v);
  EXPECT_GE(view, v);
  EXPECT_GT(view, v);
  EXPECT_LE(v, view);
  EXPECT_LT(v, view);
}


TEST_F(uri_view, hash)
{
  std::hash<sal::uri::view_t> h;
  std::set<size_t> hashes;

  auto v = view;
  hashes.insert(h(v));
  EXPECT_EQ(1U, hashes.size());

  v.scheme = "t";
  hashes.insert(h(v));
  EXPECT_EQ(2U, hashes.size());

  v.user_info = "v";
  hashes.insert(h(v));
  EXPECT_EQ(3U, hashes.size());

  v.host = "i";
  hashes.insert(h(v));
  EXPECT_EQ(4U, hashes.size());

  v.port = "2";
  hashes.insert(h(v));
  EXPECT_EQ(5U, hashes.size());

  v.path = "/q";
  hashes.insert(h(v));
  EXPECT_EQ(6U, hashes.size());

  v.query = "r";
  hashes.insert(h(v));
  EXPECT_EQ(7U, hashes.size());

  v.fragment = "g";
  hashes.insert(h(v));
  EXPECT_EQ(8U, hashes.size());
}


//}}}1


} // namespace
