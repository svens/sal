#pragma once
#include <gmock/gmock.h>
#include <sal/service/service_base.hpp>


namespace sal_test {


#if __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Weffc++"
#endif


struct service_event_handler_mock_t
  : public sal::service::service_base_t::event_handler_t
{
  MOCK_METHOD0(service_start,
    void()
  );

  MOCK_METHOD0(service_stop,
    void()
  );

  MOCK_METHOD1(service_tick,
    void(const sal::time_t&)
  );
};


#if __GNUC__
  #pragma GCC diagnostic pop
#endif


} // namespace sal_test
