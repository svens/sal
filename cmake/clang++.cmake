#
# clang++ options
#

# Compatible with g++
include(cmake/g++.cmake)

# Dependencies required for linking executables
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  list(APPEND SAL_DEP_LIBS atomic)
endif()
