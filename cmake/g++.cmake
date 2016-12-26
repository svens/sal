#
# GNU G++ options
#

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Weffc++ -Werror -pedantic -pipe")
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -ggdb -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

if("${CMAKE_GENERATOR}" STREQUAL "Ninja")
  # Ninja redirects build output and prints it only on error
  # Redirection strips colorization, so let's force it here
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
endif()


#
# Test coverage related stuff
#

if(SAL_COVERAGE)
  message(STATUS "Unittests coverage build")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage")

  find_program(LCOV lcov)
  if(NOT LCOV)
    message(FATAL_ERROR "Executable lcov not found")
  endif()
  set(LCOV_ARGS
    --quiet
    --directory ${PROJECT_SOURCE_DIR}
    --rc lcov_branch_coverage=1
  )

  if(DEFINED ENV{GCOV})
    set(GCOV_TOOL $ENV{GCOV})
  else()
    set(GCOV_TOOL gcov)
  endif()
  find_program(GCOV ${GCOV_TOOL})
  if (NOT GCOV)
    message(FATAL_ERROR "Executable $ENV{GCOV} not found")
  endif()
  set(LCOV_ARGS ${LCOV_ARGS} --gcov-tool ${GCOV})

  find_program(GENHTML genhtml)
  if(NOT GENHTML)
    message(FATAL_ERROR "Executable genhtml not found")
  endif()

  add_custom_target(gen-cov
    DEPENDS unittests
    COMMENT "Generate coverage information"

    # Initialize coverage generation
    COMMAND ${LCOV} ${LCOV_ARGS} --zerocounters
    COMMAND ${LCOV} ${LCOV_ARGS} --initial --capture --no-external --derive-func-data --output-file sal-base.info

    # Run and extract
    COMMAND ${CMAKE_COMMAND} --build . --target test
    COMMAND ${LCOV} ${LCOV_ARGS} --capture --no-external --derive-func-data --output-file sal-tests.info
    COMMAND ${LCOV} ${LCOV_ARGS} --add-tracefile sal-base.info --add-tracefile sal-tests.info --output-file sal.info
    COMMAND ${LCOV} ${LCOV_ARGS} --remove sal.info '*.test.?pp' '*/gtest/*' --output-file sal.info
    COMMAND ${LCOV} ${LCOV_ARGS} --list sal.info
    COMMAND ${LCOV} ${LCOV_ARGS} --summary sal.info
  )

  add_custom_command(TARGET gen-cov POST_BUILD
    COMMENT "Open ${CMAKE_BINARY_DIR}/cov/index.html in your browser"
    COMMAND ${GENHTML} --rc lcov_branch_coverage=1 -q --demangle-cpp --legend --output-directory cov sal.info
  )
endif()
