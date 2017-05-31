#pragma once

#include <sal/common.test.hpp>


namespace sal_test {


template <typename Protocol>
inline std::string to_s (::testing::TestParamInfo<Protocol> p)
{
  return p.param == Protocol::v4() ? "v4" : "v6";
}


} // namespace sal_test
