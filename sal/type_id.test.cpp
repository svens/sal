#include <sal/type_id.hpp>
#include <sal/common.test.hpp>
#include <set>


extern uintptr_t type_id_in_unit_a ();
extern uintptr_t type_id_in_unit_b ();
extern uintptr_t type_v_in_unit_a ();
extern uintptr_t type_v_in_unit_b ();


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

  EXPECT_NE(sal::type_v<char>, sal::type_v<signed char>);
  EXPECT_NE(sal::type_v<char>, sal::type_v<unsigned char>);
  EXPECT_NE(sal::type_v<signed char>, sal::type_v<unsigned char>);
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

  EXPECT_EQ(sal::type_v<ns_a::foo>, sal::type_v<ns_b::foo>);
  EXPECT_EQ(sal::type_v<ns_a::bar>, sal::type_v<ns_b::bar>);
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

  EXPECT_NE(sal::type_v<ns_a::foo>, sal::type_v<ns_c::foo>);
  EXPECT_NE(sal::type_v<ns_a::bar>, sal::type_v<ns_c::bar>);
}


TEST(type_id, same_in_different_compilation_units)
{
  EXPECT_EQ(type_id_in_unit_a(), type_id_in_unit_b());
  EXPECT_EQ(type_v_in_unit_a(), type_v_in_unit_b());
}


TEST(type_id, unique_type_id_for_type)
{
  std::set<uintptr_t> type_set
  {
    // POD types
    sal::type_id<bool>(),
    sal::type_id<char>(),
    sal::type_id<signed char>(),
    sal::type_id<unsigned char>(),
    sal::type_id<short>(),               // 5
    sal::type_id<unsigned short>(),
    sal::type_id<int>(),
    sal::type_id<unsigned int>(),
    sal::type_id<long>(),
    sal::type_id<unsigned long>(),       // 10
    sal::type_id<long long>(),
    sal::type_id<unsigned long long>(),
    sal::type_id<float>(),
    sal::type_id<double>(),
    sal::type_id<long double>(),         // 15

    // pointer types
    sal::type_id<bool *>(),
    sal::type_id<char *>(),
    sal::type_id<signed char *>(),
    sal::type_id<unsigned char *>(),
    sal::type_id<short *>(),             // 20
    sal::type_id<unsigned short *>(),
    sal::type_id<int *>(),
    sal::type_id<unsigned int *>(),
    sal::type_id<long *>(),
    sal::type_id<unsigned long *>(),     // 25
    sal::type_id<long long *>(),
    sal::type_id<unsigned long long *>(),
    sal::type_id<float *>(),
    sal::type_id<double *>(),
    sal::type_id<long double *>(),       // 30

    // types in namespaces
    sal::type_id<ns_a::foo>(),
    sal::type_id<ns_b::foo>(),           // 31: same as ns_a::foo
    sal::type_id<ns_c::foo>(),

    // alias types
    sal::type_id<ns_a::bar>(),           // 33: same as int
    sal::type_id<ns_b::bar>(),           // 33: same as int
    sal::type_id<ns_c::bar>(),           // 33: same as short
  };
  EXPECT_EQ(32U, type_set.size());
}


TEST(type_id, unique_type_id_for_var)
{
  bool bool_{};
  char char_{};
  signed char signed_char_{};
  unsigned char unsigned_char_{};
  short short_{};
  unsigned short unsigned_short_{};
  int int_{};
  unsigned int unsigned_int_{};
  long long_{};
  unsigned long unsigned_long_{};
  long long long_long_{};
  unsigned long long unsigned_long_long_{};
  float float_{};
  double double_{};
  long double long_double_{};
  bool *bool_p_{};
  char *char_p_{};
  signed char *signed_char_p_{};
  unsigned char *unsigned_char_p_{};
  short *short_p_{};
  unsigned short *unsigned_short_p_{};
  int *int_p_{};
  unsigned int *unsigned_int_p_{};
  long *long_p_{};
  unsigned long *unsigned_long_p_{};
  long long *long_long_p_{};
  unsigned long long *unsigned_long_long_p_{};
  float *float_p_{};
  double *double_p_{};
  long double *long_double_p_{};
  ns_a::foo a_foo_{};
  ns_b::foo b_foo_{};
  ns_c::foo c_foo_{};
  ns_a::bar a_bar_{};
  ns_b::bar b_bar_{};
  ns_c::bar c_bar_{};

  std::set<uintptr_t> type_set
  {
    // POD types
    sal::type_id(bool_),
    sal::type_id(char_),
    sal::type_id(signed_char_),
    sal::type_id(unsigned_char_),
    sal::type_id(short_),               // 5
    sal::type_id(unsigned_short_),
    sal::type_id(int_),
    sal::type_id(unsigned_int_),
    sal::type_id(long_),
    sal::type_id(unsigned_long_),       // 10
    sal::type_id(long_long_),
    sal::type_id(unsigned_long_long_),
    sal::type_id(float_),
    sal::type_id(double_),
    sal::type_id(long_double_),         // 15

    // pointer types
    sal::type_id(bool_p_),
    sal::type_id(char_p_),
    sal::type_id(signed_char_p_),
    sal::type_id(unsigned_char_p_),
    sal::type_id(short_p_),             // 20
    sal::type_id(unsigned_short_p_),
    sal::type_id(int_p_),
    sal::type_id(unsigned_int_p_),
    sal::type_id(long_p_),
    sal::type_id(unsigned_long_p_),     // 25
    sal::type_id(long_long_p_),
    sal::type_id(unsigned_long_long_p_),
    sal::type_id(float_p_),
    sal::type_id(double_p_),
    sal::type_id(long_double_p_),       // 30

    // types in namespaces
    sal::type_id(a_foo_),
    sal::type_id(b_foo_),               // 31: same as ns_a::foo
    sal::type_id(c_foo_),

    // alias types
    sal::type_id(a_bar_),               // 33: same as int
    sal::type_id(b_bar_),               // 33: same as int
    sal::type_id(c_bar_),               // 33: same as short
  };
  EXPECT_EQ(32U, type_set.size());
}


TEST(type_id, unique_type_v)
{
  std::set<uintptr_t> type_set
  {
    // POD types
    sal::type_v<bool>,
    sal::type_v<char>,
    sal::type_v<signed char>,
    sal::type_v<unsigned char>,
    sal::type_v<short>,                 // 5
    sal::type_v<unsigned short>,
    sal::type_v<int>,
    sal::type_v<unsigned int>,
    sal::type_v<long>,
    sal::type_v<unsigned long>,         // 10
    sal::type_v<long long>,
    sal::type_v<unsigned long long>,
    sal::type_v<float>,
    sal::type_v<double>,
    sal::type_v<long double>,           // 15

    // pointer types
    sal::type_v<bool *>,
    sal::type_v<char *>,
    sal::type_v<signed char *>,
    sal::type_v<unsigned char *>,
    sal::type_v<short *>,               // 20
    sal::type_v<unsigned short *>,
    sal::type_v<int *>,
    sal::type_v<unsigned int *>,
    sal::type_v<long *>,
    sal::type_v<unsigned long *>,       // 25
    sal::type_v<long long *>,
    sal::type_v<unsigned long long *>,
    sal::type_v<float *>,
    sal::type_v<double *>,
    sal::type_v<long double *>,         // 30

    // types in namespaces
    sal::type_v<ns_a::foo>,
    sal::type_v<ns_b::foo>,             // 31: same as ns_a::foo
    sal::type_v<ns_c::foo>,

    // alias types
    sal::type_v<ns_a::bar>,             // 33: same as int
    sal::type_v<ns_b::bar>,             // 33: same as int
    sal::type_v<ns_c::bar>,             // 33: same as short
  };
  EXPECT_EQ(32U, type_set.size());
}


} // namespace
