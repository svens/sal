cmake_minimum_required(VERSION 3.2)

project(sal)
include(cmake/project_version.cmake)
message(STATUS "sal: ${PROJECT_VERSION}")


# Settings {{{1

# optional features
option(sal_unittests "Build unittests" ON)
option(sal_bench "Build benchmarking application" OFF)
option(sal_docs "Generate documentation" OFF)

if(CMAKE_BUILD_TYPE MATCHES Coverage)
  # special case of coverage build
  set(CMAKE_BUILD_TYPE "Debug")
  set(COVERAGE ON)
  set(sal_unittests ON)
  set(sal_bench OFF)
endif()


# Build targets {{{1

# generate config header
message(STATUS "Generate sal/config.hpp")
configure_file(
  ${PROJECT_SOURCE_DIR}/sal/config.hpp.in
  ${CMAKE_BINARY_DIR}/sal/config.hpp
)
set_source_files_properties(${CMAKE_BINARY_DIR}/sal/config.hpp
  PROPERTIES GENERATED TRUE
)

# collect sources from sal/
set(sal_sources "${CMAKE_BINARY_DIR}/sal/config.hpp")
set(sal_unittests_sources "")
include(sal/list.cmake)

# set compiler flags
include_directories(. ${CMAKE_BINARY_DIR})
if(CMAKE_COMPILER_IS_GNUCXX)
  include(cmake/g++.cmake)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  include(cmake/clang++.cmake)
elseif(MSVC)
  include(cmake/msvc.cmake)
else()
  message(WARNING "Unknown compiler")
endif()

# set platform flags
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  include(cmake/macos.cmake)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  include(cmake/linux.cmake)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  include(cmake/windows.cmake)
else()
  message(WARNING "Unknown system")
endif()

# create static library
add_library(sal ${sal_sources})


# Testing targets {{{1

if(sal_unittests)
  message(STATUS "Building unittests")
  include(cmake/gtest.cmake)

  enable_testing()

  add_executable(unittests ${sal_unittests_sources})
  target_link_libraries(unittests
    sal
    gtest
    gmock_main
    ${SAL_DEP_LIBS}
  )
  add_dependencies(unittests gtest)

  add_test(unittests unittests)
endif()


# Benchmarking application {{{1

if(sal_bench)
  message(STATUS "Building benchmarking applications")

  find_package(Threads)

  set(sal_bench_sources "")
  set(sal_bench_modules "")
  include(bench/list.cmake)

  foreach (module ${sal_bench_modules})
    get_filename_component(name ${module} NAME_WE)
    set(bench_name "bench_${name}")
    add_executable(${bench_name}
      ${sal_bench_sources}
      ${module}
    )
    target_link_libraries(${bench_name}
      sal
      ${CMAKE_THREAD_LIBS_INIT}
      ${SAL_DEP_LIBS}
    )
  endforeach()
endif()


# Generate documentation {{{1

if(sal_docs)
  message(STATUS "Generate documentation")
  include(cmake/doxygen.cmake)
endif()


# Package creation {{{1

set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
list(APPEND CPACK_GENERATOR "TGZ")

# libraries
install(TARGETS sal
  ARCHIVE
  DESTINATION lib
  COMPONENT libraries
)
list(APPEND CPACK_COMPONENTS_ALL libraries)

# headers
foreach(header ${sal_sources})
  get_filename_component(file_ext ${header} EXT)
  if(${file_ext} STREQUAL ".hpp")
    get_filename_component(file_path ${header} PATH)
    string(REPLACE "${CMAKE_BINARY_DIR}/" "" file_path ${file_path})
    install(FILES ${header}
      DESTINATION include/${file_path}
      COMPONENT headers
    )
  endif()
endforeach()

list(APPEND CPACK_COMPONENT_ALL headers)
set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)

include(CPack)
