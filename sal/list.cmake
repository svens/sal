# sources
list(APPEND sal_sources
  sal/atomic_queue.hpp
  sal/assert.hpp
  sal/builtins.hpp
  sal/c_str.hpp
  sal/fmtval.hpp
  sal/__bits/fmtval.hpp
  sal/spinlock.hpp

  # temporary file to let cmake detect library language
  sal/void.cpp
)


# unittests
list(APPEND sal_unittests
  sal/atomic_queue.test.cpp
  sal/assert.test.cpp
  sal/builtins.test.cpp
  sal/c_str.test.cpp
  sal/common.test.hpp
  sal/fmtval.test.cpp
  sal/spinlock.test.cpp
)
