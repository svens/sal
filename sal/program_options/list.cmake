# sources
list(APPEND sal_sources
  sal/program_options/argument_map.hpp
  sal/program_options/command_line.hpp
  sal/program_options/command_line.cpp
  sal/program_options/config_reader.hpp
  sal/program_options/config_reader.cpp
  sal/program_options/error.hpp
  sal/program_options/option_set.hpp
  sal/program_options/option_set.cpp
)


# unittests
list(APPEND sal_unittests
  sal/program_options/common.test.hpp
  sal/program_options/command_line.test.cpp
  sal/program_options/config_reader.test.cpp
  sal/program_options/option_set.test.cpp
)
