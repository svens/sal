# sources
list(APPEND sal_sources
  sal/assert.hpp
  sal/builtins.hpp
  sal/error.hpp
  sal/file.hpp
  sal/file.cpp
  sal/fmt.hpp
  sal/__bits/fmt.hpp
  sal/queue.hpp
  sal/__bits/queue_intrusive.hpp
  sal/__bits/queue_mpsc.hpp
  sal/__bits/queue_spsc.hpp
  sal/spinlock.hpp
  sal/str.hpp
  sal/thread.hpp
  sal/thread.cpp
  sal/time.hpp
)


# unittests
list(APPEND sal_unittests
  sal/assert.test.cpp
  sal/builtins.test.cpp
  sal/common.test.hpp
  sal/error.test.cpp
  sal/file.test.cpp
  sal/fmt.test.cpp
  sal/queue.test.cpp
  sal/spinlock.test.cpp
  sal/str.test.cpp
  sal/thread.test.cpp
  sal/time.test.cpp
)


# submodules
include(sal/logger/list.cmake)
include(sal/program_options/list.cmake)
