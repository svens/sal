find_package(Threads)
find_package(GTest QUIET)

if(NOT GTEST_FOUND)
  message(STATUS "Building own gtest")

  # download and build
  include(ExternalProject)
  ExternalProject_Add(gtest
    URL https://github.com/google/googletest/archive/release-1.7.0.zip
    URL_HASH SHA256=b58cb7547a28b2c718d1e38aee18a3659c9e3ff52440297e965f5edffe34b6d0
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest-1.7.0
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=Release
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
