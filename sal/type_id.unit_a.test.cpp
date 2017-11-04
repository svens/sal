#include <sal/type_id.hpp>
#include <sal/common.test.hpp>


uintptr_t type_id_in_unit_a () noexcept
{
  return sal::type_id<sal_test::fixture>();
}


uintptr_t type_v_in_unit_a () noexcept
{
  return sal::type_v<sal_test::fixture>;
}
