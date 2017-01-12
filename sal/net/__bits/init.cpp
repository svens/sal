#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <mutex>

#if __sal_os_windows
  #include <winsock2.h>
#else
  #include <signal.h>
#endif


__sal_begin


namespace net {


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
#if __sal_os_windows

  WSADATA wsa;
  result.assign(
    ::WSAStartup(MAKEWORD(2, 2), &wsa),
    std::system_category()
  );

#else

  (void)result;
  ::signal(SIGPIPE, SIG_IGN);

#endif
}


void lib_t::setup () noexcept
{
  static std::once_flag flag;
  std::call_once(flag, &internal_setup, std::ref(setup_result));
}


void lib_t::cleanup () noexcept
{
#if __sal_os_windows
  ::WSACleanup();
#endif
}


} // namespace


const std::error_code &init () noexcept
{
  lib_t::setup();
  return lib_t::setup_result;
}


} // namespace net


__sal_end
