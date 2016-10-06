#include <bench/bench.hpp>
#include <sal/fmt.hpp>
#include <sal/str.hpp>
#include <cinttypes>
#include <iostream>


namespace {


std::string function = "str";
size_t count = 10'000'000;


const bool p_bool = true;
const char p_char = 'a';
const signed char p_schar = 'b';
const unsigned char p_uchar = 'c';
const int16_t p_i16 = 12;
const uint16_t p_u16 = 23;
const int32_t p_i32 = 34;
const uint32_t p_u32 = 45;
const int64_t p_i64 = 56;
const uint64_t p_u64 = 67;
const float p_float = 7.8f;
const double p_double = 8.9;
const long double p_ldouble = 9.1;
const void *p_ptr = &p_bool;
const char *p_cstr = "hello, world";
const std::string p_str = "goodbye, world";


template <typename F>
int worker (F f)
{
  size_t current = 0, percent = 0;

  auto start_time = bench::start();
  while (bench::in_progress(++current, count, percent))
  {
    f();
  }
  bench::stop(start_time, count);

  return EXIT_SUCCESS;
}


void use_printf ()
{
  char buf[1024];
  snprintf(buf, sizeof(buf),
    "bool=%s"
    "; char=%c"
    "; schar=%c"
    "; uchar=%c"
    "; i16=%hd"
    "; u16=%hu"
    "; u16:o=%ho"
    "; u16:h=%hx"
    "; i32=%d"
    "; u32=%u"
    "; u32:o=%o"
    "; u32:h=%x"
    "; i64=%" PRId64
    "; u64=%" PRIu64
    "; u64:o=%" PRIo64
    "; u64:h=%" PRIx64
    "; float=%g"
    "; double=%g"
    "; ldouble=%Lg"
    "; ptr=%p"
    "; str='%s'"
    "; str='%s'",
    (p_bool ? "true" : "false"),
    p_char,
    p_schar,
    p_uchar,
    p_i16,
    p_u16,
    p_u16,
    p_u16,
    p_i32,
    p_u32,
    p_u32,
    p_u32,
    p_i64,
    p_u64,
    p_u64,
    p_u64,
    p_float,
    p_double,
    p_ldouble,
    p_ptr,
    p_cstr,
    p_str.c_str()
  );

  if (count == 1)
  {
    printf("%s\n", buf);
  }
}


void use_str ()
{
  sal::str_t<1024> str;
  str << "bool=" << p_bool
    << "; char=" << p_char
    << "; schar=" << p_schar
    << "; uchar=" << p_uchar
    << "; i16=" << p_i16
    << "; u16=" << p_u16
    << "; u16:o=" << sal::oct(p_u16)
    << "; u16:h=" << sal::hex(p_u16)
    << "; i32=" << p_i32
    << "; u32=" << p_u32
    << "; u32:o=" << sal::oct(p_u32)
    << "; u32:h=" << sal::hex(p_u32)
    << "; i64=" << p_i64
    << "; u64=" << p_u64
    << "; u64:o=" << sal::oct(p_u64)
    << "; u64:h=" << sal::hex(p_u64)
    << "; float=" << p_float
    << "; double=" << p_double
    << "; ldouble=" << p_ldouble
    << "; ptr=" << p_ptr
    << "; str='" << p_cstr << '\''
    << "; str='" << p_str << '\''
  ;

  if (count == 1)
  {
    printf("%s\n", str.get());
  }
}


} // namespace


namespace bench {


option_set_t options ()
{
  using namespace sal::program_options;

  option_set_t desc;
  desc
    .add({"c", "count"},
      requires_argument("INT", count),
      help("number of iterations")
    )
    .add({"f", "function"},
      requires_argument("STRING", function),
      help("function to test (str | printf)")
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  count = std::stoul(options.back_or_default("count", { arguments }));
  function = options.back_or_default("function", { arguments });

  if (function == "str")
  {
    return worker(use_str);
  }
  else if (function == "printf")
  {
    return worker(use_printf);
  }

  return usage("unknown function '" + function + '\'');
}


} // namespace bench
