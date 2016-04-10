#pragma once

#include <string>
#include <unordered_map>
#include <vector>


namespace bench {

// list of benchmark functions: name -> func_ptr
using arg_list = std::vector<std::string>;
using func_ptr = int(*)(const arg_list &);
using func_list = std::unordered_map<std::string, func_ptr>;

} // namespace bench
