list(APPEND sal_sources
  sal/assert.hpp
  sal/builtins.hpp
  sal/fmtval.hpp
  sal/__bits/fmtval.hpp
  sal/view.hpp

  # temporary file to let cmake detect library language
  sal/void.cpp
)

list(APPEND sal_unittests
  sal/assert.test.cpp
  sal/builtins.test.cpp
  sal/common.test.hpp
  sal/fmtval.test.cpp
  sal/view.test.cpp
)
