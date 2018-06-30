#
# MSVC options
#

set(CMAKE_CXX_FLAGS "/nologo /std:c++latest /EHsc /W4 /WX")
set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /Zi /Od /RTC1 /sdl /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "/DNDEBUG /Ox /MT")

add_definitions(
  /D _SBCS
  /D WIN32_LEAN_AND_MEAN
)

# https://docs.microsoft.com/en-us/cpp/preprocessor/compiler-warnings-that-are-off-by-default
string(CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}"
  " /w34265"
  " /w34777"
  " /w34946"
  " /w35038"
)
