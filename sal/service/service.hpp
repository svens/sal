#pragma once

/**
 * \file sal/service/service.cpp
 */

#include <sal/config.hpp>
#include <sal/service/service_base.hpp>
#include <sal/program_options/option_set.hpp>


#include <iostream>


__sal_begin


namespace service {


template <typename Application>
class service_t
  : public service_base_t
{
public:

  service_t (int argc, const char *argv[],
      program_options::option_set_t options)
    : service_base_t(argc, argv, options)
  {}


  service_t (int argc, const char *argv[])
    : service_t(argc, argv, {})
  {}


  int run () noexcept
  {
    return service_base_t::run();
  }
};


} // namespace service


__sal_end
