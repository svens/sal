#pragma once

#include <sal/program_options/argument_map.hpp>
#include <sal/program_options/option_set.hpp>
#include <chrono>


namespace bench {


//
// benchmark measuring and progress printing
// in bench/main.cpp
//

using clock_type = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<clock_type>;
using milliseconds = std::chrono::milliseconds;

time_point start ();
milliseconds stop (time_point start_time, size_t total);
bool in_progress (size_t current, size_t total, size_t &percent);

int usage (const std::string message);


//
// benchmarking setup/running/teardown
// in module-specific .pp
//

using sal::program_options::option_set_t;
using sal::program_options::argument_map_t;

option_set_t options ();
int run (const option_set_t &options, const argument_map_t &arguments);


} // namespace bench
