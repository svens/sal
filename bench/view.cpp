#include <bench/bench.hpp>
#include <sal/view.hpp>
#include <chrono>
#include <iostream>


namespace {


using namespace std::chrono;
using clock_type = high_resolution_clock;


std::string func = "view";
size_t count = 10'000'000;


int usage (const std::string message="")
{
  if (!message.empty())
  {
    std::cerr << message << '\n' << std::endl;
  }

  std::cerr << "view:"
    << "\n  --help        this page"
    << "\n  --count=int   number of iterations"
    << "\n  --func=Func   function to test"
    << "\n                possible values: view, printf"
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
  auto start = clock_type::now();

  std::cout << "[          ]\r[" << std::flush;
  auto centile = count/10;
  for (auto i = count;  i;  --i)
  {
    f();
    if (i % centile == 0)
    {
      std::cout << '=' << std::flush;
    }
  }

  auto delay = clock_type::now() - start;
  auto ms = duration_cast<milliseconds>(delay).count();
  if (!ms)
  {
    ms = 1;
  }

  std::cout << '\n' << ms << " msec"
    << ", " << count/ms << " count/msec"
    << std::endl;

  return EXIT_SUCCESS;
}


void use_printf ()
{
  char buf[1024];
  snprintf(buf, sizeof(buf),
    "prefix {%s, %c, %c, %c, %hd, %hu, %d, %u, %lld, %llu, %g, %g, %Lg, %p, %s, %s} suffix\n",
    (p_bool ? "true" : "false"),
    p_char,
    p_schar,
    p_uchar,
    p_i16,
    p_u16,
    p_i32,
    p_u32,
    p_i64,
    p_u64,
    p_float,
    p_double,
    p_ldouble,
    p_ptr,
    p_cstr,
    p_str.c_str()
  );
}


void use_view ()
{
  sal::view<1024> view;
  view << "prefix {" << p_bool
    << ", " << p_char
    << ", " << p_schar
    << ", " << p_uchar
    << ", " << p_i16
    << ", " << p_u16
    << ", " << p_i32
    << ", " << p_u32
    << ", " << p_i64
    << ", " << p_u64
    << ", " << p_float
    << ", " << p_double
    << ", " << p_ldouble
    << ", " << p_ptr
    << ", " << p_cstr
    << ", " << p_str
    << "} suffix\n"
  ;
}


} // namespace


int bench::view (const arg_list &args)
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

  if (func == "view")
  {
    return worker(use_view);
  }
  else if (func == "printf")
  {
    return worker(use_printf);
  }

  return usage("unknown func '" + func + '\'');
}
