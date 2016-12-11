project(sal)
cmake_minimum_required(VERSION 2.8.10)


# Settings {{{1

# optional features
option(SAL_UNITTESTS "Build unittests" ON)
option(SAL_BENCH "Build benchmarking application" OFF)
option(SAL_DOCS "Generate documentation" OFF)

if(CMAKE_BUILD_TYPE MATCHES Coverage)
  # special case of coverage build
  set(CMAKE_BUILD_TYPE "Debug")
  set(SAL_COVERAGE ON)
  set(SAL_UNITTESTS ON)
  set(SAL_BENCH OFF)
endif()


# Version {{{1

# read and parse
file(STRINGS VERSION version LIMIT_COUNT 1)
set(version_regex "([0-9]*)\\.([0-9]*)\\.([0-9]*)")
string(REGEX REPLACE ${version_regex} "\\1" version_major ${version})
math(EXPR v ${version_major}+0)
string(REGEX REPLACE ${version_regex} "\\2" version_minor ${version})
math(EXPR v ${version_minor}+0)
string(REGEX REPLACE ${version_regex} "\\3" version_patch ${version})
math(EXPR v ${version_patch}+0)

# generate config header
message(STATUS "Generate sal/config.hpp")
configure_file(
  ${PROJECT_SOURCE_DIR}/sal/config.hpp.in
  ${CMAKE_BINARY_DIR}/sal/config.hpp
)
set_source_files_properties(${CMAKE_BINARY_DIR}/sal/config.hpp
  PROPERTIES GENERATED TRUE
)


# Build targets {{{1

# collect sources from sal/
set(sal_sources "")
set(sal_unittests "")
set(sal_docs "")
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
  message(WARNING "Unknown compiler, not setting flags")
endif()

# create static library
add_library(sal ${sal_sources})


# Testing targets {{{1

if(SAL_UNITTESTS)
  message(STATUS "Building unittests")

  find_package(GTest QUIET)
  find_package(Threads)
  if(NOT GTEST_FOUND)
    message(STATUS "Building own gtest")

    # download and build
    include(ExternalProject)
    ExternalProject_Add(gtest
      URL https://github.com/google/googletest/archive/release-1.7.0.zip
      URL_HASH SHA1=f89bc9f55477df2fde082481e2d709bfafdb057b
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest-1.7.0
      CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      INSTALL_COMMAND ""
      LOG_DOWNLOAD ON
      LOG_CONFIGURE ON
      LOG_BUILD ON
    )

    # gtest headers
    ExternalProject_Get_Property(gtest source_dir)
    set(GTEST_INCLUDE_DIRS ${source_dir}/include)

    # gtest libs
    ExternalProject_Get_Property(gtest binary_dir)
    link_directories(${binary_dir})
    set(GTEST_BOTH_LIBRARIES
      ${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
  endif()

  # build sal unittests
  enable_testing()
  add_executable(unittests ${sal_unittests})
  include_directories(${GTEST_INCLUDE_DIRS})
  if(NOT GTEST_FOUND)
    add_dependencies(unittests gtest)
  endif()

  # relax compiler warnings for tests, intentionally creating unhappy paths
  if(CMAKE_COMPILER_IS_GNUCXX)
    set_source_files_properties(${sal_unittests}
      PROPERTIES COMPILE_FLAGS "-Wno-effc++"
    )
  elseif(MSVC)
    set_source_files_properties(${sal_unittests}
      PROPERTIES COMPILE_FLAGS "/analyze-"
    )
  endif()
  target_link_libraries(unittests
    sal
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${SAL_DEP_LIBS}
  )
  add_test(unittests unittests)
endif()


# Benchmarking application {{{1

if(SAL_BENCH)
  message(STATUS "Building benchmarking applications")

  find_package(Threads)

  set(sal_bench "")
  set(sal_bench_modules "")
  include(bench/list.cmake)

  foreach (module ${sal_bench_modules})
    get_filename_component(name ${module} NAME_WE)
    set(bench_name "bench_${name}")
    message("   - ${bench_name}")
    add_executable(${bench_name}
      ${sal_bench}
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

if(SAL_DOCS)
  message(STATUS "Generate documentation")

  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    configure_file(
      ${PROJECT_SOURCE_DIR}/cmake/Doxyfile.in
      ${CMAKE_BINARY_DIR}/Doxyfile
    )
    add_custom_target(gen-doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Generating documentation"
    )
  else()
    message(WARNING "Doxygen not found, skipping documentation generation")
  endif()
endif()


# Package creation {{{1

set(CPACK_PACKAGE_VERSION "${version}")
set(CPACK_PACKAGE_VERSION_MAJOR "${version_major}")
set(CPACK_PACKAGE_VERSION_MINOR "${version_minor}")
set(CPACK_PACKAGE_VERSION_PATCH "${version_patch}")

set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")

# binary packages
set(CPACK_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${version}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}"
)
list(APPEND CPACK_GENERATOR "TGZ")

# source packages
set(CPACK_SOURCE_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${version}-src"
)
list(APPEND CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  .git
  .gitignore
  .*.swp
  build
  tags
)

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
  if("${file_ext}" STREQUAL ".hpp")
    get_filename_component(file_path ${header} PATH)
    install(FILES ${header}
      DESTINATION include/${file_path}
      COMPONENT headers
    )
  endif()
endforeach()

# autogenerated files
install(FILES
  "${CMAKE_BINARY_DIR}/sal/config.hpp"
  DESTINATION include/sal
  COMPONENT headers
)

list(APPEND CPACK_COMPONENT_ALL headers)
set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)

include(CPack)
