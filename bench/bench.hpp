#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>


namespace bench {


// list of benchmark functions: name -> func_ptr
using arg_list = std::vector<std::string>;
using func_ptr = int(*)(const arg_list &);
using func_list = std::unordered_map<std::string, func_ptr>;


//
// benchmark functions
//

int c_str (const arg_list &args);
int concurrent_queue (const arg_list &args);
int spinlock (const arg_list &args);


//
// benchmark measuring and progress printing
//

using clock_type = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<clock_type>;
using milliseconds = std::chrono::milliseconds;

time_point start ();
milliseconds stop (time_point start_time, size_t total);

bool in_progress (size_t current, size_t total, size_t &percent);


} // namespace bench
