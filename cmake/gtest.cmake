find_package(Git)
execute_process(
  COMMAND ${GIT_EXECUTABLE} submodule update --init tps/googletest
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(tps/googletest)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tps/googletest/include)

# relax compiler checks for tests, intentionally creating unappy paths
if(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  target_compile_options(gtest
    PUBLIC -Wno-effc++ -Wno-extra
  )
elseif(MSVC)
  target_compile_options(gtest
    PUBLIC /DGTEST_HAS_TR1_TUPLE=0 /DGTEST_HAS_STD_TUPLE=1 /analyze-
  )
endif()
