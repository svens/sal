#pragma once

#include <sal/config.hpp>
#include <system_error>

#if __sal_os_darwin //{{{1

#elif __sal_os_linux //{{{1

#elif __sal_os_windows //{{{1

#endif //}}}1


__sal_begin


namespace crypto { namespace __bits {


#if __sal_os_darwin //{{{1

#elif __sal_os_linux //{{{1

#elif __sal_os_windows //{{{1

#endif //}}}1


}} // namespace crypto::__bits


__sal_end
