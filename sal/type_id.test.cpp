#include <sal/type_id.hpp>
#include <sal/common.test.hpp>


extern uintptr_t type_in_unit_a ();
extern uintptr_t type_in_unit_b ();


namespace {


TEST(type_id, character_types)
{
  EXPECT_NE(sal::type_id<char>(), sal::type_id<signed char>());
  EXPECT_NE(sal::type_id<char>(), sal::type_id<unsigned char>());
  EXPECT_NE(sal::type_id<signed char>(), sal::type_id<unsigned char>());

  char a{};
  signed char b{};
  unsigned char c{};
  EXPECT_NE(sal::type_id(a), sal::type_id(b));
  EXPECT_NE(sal::type_id(a), sal::type_id(c));
  EXPECT_NE(sal::type_id(b), sal::type_id(c));
}


namespace ns_a {
  struct foo {};
  using bar = int;
}

namespace ns_b {
  using foo = ns_a::foo;
  using bar = int;
}

namespace ns_c {
  struct foo {};
  using bar = short;
}


TEST(type_id, same_in_different_namespace)
{
  EXPECT_EQ(sal::type_id<ns_a::foo>(), sal::type_id<ns_b::foo>());
  EXPECT_EQ(sal::type_id<ns_a::bar>(), sal::type_id<ns_b::bar>());

  ns_a::foo a_foo{};
  ns_a::bar a_bar{};
  ns_b::foo b_foo{};
  ns_b::bar b_bar{};
  EXPECT_EQ(sal::type_id(a_foo), sal::type_id(b_foo));
  EXPECT_EQ(sal::type_id(a_bar), sal::type_id(b_bar));
}


TEST(type_id, different_in_different_namespace)
{
  EXPECT_NE(sal::type_id<ns_a::foo>(), sal::type_id<ns_c::foo>());
  EXPECT_NE(sal::type_id<ns_a::bar>(), sal::type_id<ns_c::bar>());

  ns_a::foo a_foo{};
  ns_a::bar a_bar{};
  ns_c::foo c_foo{};
  ns_c::bar c_bar{};
  EXPECT_NE(sal::type_id(a_foo), sal::type_id(c_foo));
  EXPECT_NE(sal::type_id(a_bar), sal::type_id(c_bar));
}


TEST(type_id, same_in_different_compilation_units)
{
  EXPECT_EQ(type_in_unit_a(), type_in_unit_b());
}


} // namespace
