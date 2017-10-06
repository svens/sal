#include <sal/type_id.hpp>
#include <sal/common.test.hpp>


uintptr_t type_in_unit_b () noexcept
{
  return sal::type_id<sal_test::fixture>();
}
