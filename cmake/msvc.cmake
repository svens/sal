#
# MSVC options
#

# all
set(CMAKE_CXX_FLAGS "/nologo /W4 /WX /EHsc")
add_definitions(
  /D _SBCS
  /D WIN32_LEAN_AND_MEAN
)

# Debug
set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /RTC1 /sdl /MTd")

# Release
set(CMAKE_CXX_FLAGS_RELEASE "/Ox /MT")

# RelWithDebInfo
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /Ox /MTd")

# MinSizeRel
set(CMAKE_CXX_FLAGS_MINSIZEREL "/Os /MT")
