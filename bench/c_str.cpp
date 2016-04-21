#include <bench/bench.hpp>
#include <sal/c_str.hpp>
#include <sal/fmtval.hpp>
#include <chrono>
#include <iostream>


namespace {


using namespace std::chrono;
using clock_type = high_resolution_clock;


std::string func = "c_str";
size_t count = 10'000'000;


int usage (const std::string message="")
{
  if (!message.empty())
  {
    std::cerr << message << '\n' << std::endl;
  }

  std::cerr << "c_str:"
    << "\n  --help        this page"
    << "\n  --count=int   number of iterations"
    << "\n  --func=Func   function to test"
    << "\n                possible values: c_str, printf"
    << std::endl;

  return EXIT_FAILURE;
}


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

  auto start = clock_type::now();
  while (bench::in_progress(current, count, percent))
  {
    f();
  }

  auto delay = clock_type::now() - start;
  auto msec = duration_cast<milliseconds>(delay).count();
  if (!msec)
  {
    msec = 1;
  }

  std::cout << '\n' << msec << " msec"
    << ", " << count/msec << " count/msec"
    << std::endl;

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
    "; i64=%lld"
    "; u64=%llu"
    "; u64:o=%llo"
    "; u64:h=%llx"
    "; float=%g"
    "; double=%g"
    "; ldouble=%Lg"
    "; ptr=%p"
    "; c_str='%s'"
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


void use_c_str ()
{
  sal::c_str<1024> c_str;
  c_str << "bool=" << p_bool
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
    << "; c_str='" << p_cstr << '\''
    << "; str='" << p_str << '\''
  ;

  if (count == 1)
  {
    printf("%s\n", c_str.get());
  }
}


} // namespace


int bench::c_str (const arg_list &args)
{
  for (auto &arg: args)
  {
    if (arg == "--help")
    {
      return usage();
    }
    else if (arg.find("--count=") != arg.npos)
    {
      count = std::stoul(arg.substr(sizeof("--count=") - 1));
    }
    else if (arg.find("--func=") != arg.npos)
    {
      func = arg.substr(sizeof("--func=") - 1);
    }
    else
    {
      return usage("unknown argument: " + arg);
    }
  }

  if (func == "c_str")
  {
    return worker(use_c_str);
  }
  else if (func == "printf")
  {
    return worker(use_printf);
  }

  return usage("unknown func '" + func + '\'');
}
