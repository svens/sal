#pragma once

#include <string>
#include <unordered_map>
#include <vector>


namespace bench {

// list of benchmark functions: name -> func_ptr
using arg_list = std::vector<std::string>;
using func_ptr = int(*)(const arg_list &);
using func_list = std::unordered_map<std::string, func_ptr>;

// benchmark functions
int c_str (const arg_list &args);
int spinlock (const arg_list &args);

// progress printing
bool in_progress (size_t current, size_t total, size_t &percent);

} // namespace bench
