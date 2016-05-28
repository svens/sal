# sources
list(APPEND sal_sources
  sal/assert.hpp
  sal/builtins.hpp
  sal/c_str.hpp
  sal/concurrent_queue.hpp
  sal/__bits/queue_mpmc.hpp
  sal/__bits/queue_mpsc.hpp
  sal/__bits/queue_spmc.hpp
  sal/__bits/queue_spsc.hpp
  sal/error.hpp
  sal/file.hpp
  sal/file.cpp
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
  sal/error.test.cpp
  sal/file.test.cpp
  sal/fmtval.test.cpp
  sal/spinlock.test.cpp
  sal/thread.test.cpp
  sal/time.test.cpp
)
