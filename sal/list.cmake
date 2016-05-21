# sources
list(APPEND sal_sources
  sal/assert.hpp
  sal/builtins.hpp
  sal/c_str.hpp
  sal/concurrent_queue.hpp
  sal/fmtval.hpp
  sal/__bits/fmtval.hpp
  sal/spinlock.hpp
  sal/thread.hpp
  sal/thread.cpp
  sal/time.hpp
)


# unittests
list(APPEND sal_unittests
  sal/concurrent_queue.test.cpp
  sal/assert.test.cpp
  sal/builtins.test.cpp
  sal/c_str.test.cpp
  sal/common.test.hpp
  sal/fmtval.test.cpp
  sal/spinlock.test.cpp
  sal/thread.test.cpp
  sal/time.test.cpp
)
