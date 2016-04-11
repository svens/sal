#
# MSVC options
#

set(CMAKE_CXX_FLAGS "/nologo /W4 /WX /EHsc")
set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /Zi /Od /RTC1 /sdl /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "/DNDEBUG /Ox /MT")

add_definitions(
  /D _SBCS
  /D WIN32_LEAN_AND_MEAN
)
