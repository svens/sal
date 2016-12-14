#include <sal/net/__bits/platform.hpp>
#include <sal/net/fwd.hpp>

#if __sal_os_windows
  #include <mutex>
#endif


__sal_begin


namespace net {


#if __sal_os_windows

namespace {


struct lib_t
{
  lib_t () noexcept
  {
    setup();
  }

  ~lib_t () noexcept
  {
    cleanup();
  }

  static std::error_code setup_result;
  static lib_t lib;

  static void setup () noexcept;
  static void cleanup () noexcept;
};

std::error_code lib_t::setup_result{};
lib_t lib_t::lib;


void internal_setup (std::error_code &result) noexcept
{
  WSADATA wsa;
  result.assign(
    ::WSAStartup(MAKEWORD(2, 2), &wsa),
    std::system_category()
  );
}


void lib_t::setup () noexcept
{
  static std::once_flag flag;
  std::call_once(flag, &internal_setup, setup_result);
}


void lib_t::cleanup () noexcept
{
  ::WSACleanup();
}


} // namespace

#endif


const std::error_code &init () noexcept
{
#if __sal_os_windows
  lib_t::setup();
  return lib_t::setup_result;
#else
  static std::error_code result{};
  return result;
#endif
}


} // namespace net


__sal_end
