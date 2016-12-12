# sources
list(APPEND sal_sources
  sal/__bits/member_assign.hpp
  sal/assert.hpp
  sal/builtins.hpp
  sal/char_array.hpp
  sal/error.hpp
  sal/file.hpp
  sal/file.cpp
  sal/format.hpp
  sal/__bits/format.hpp
  sal/intrusive_queue.hpp
  sal/__bits/intrusive_queue.hpp
  sal/lockable.hpp
  sal/memory_writer.hpp
  sal/queue.hpp
  sal/__bits/queue.hpp
  sal/spinlock.hpp
  sal/sync_policy.hpp
  sal/thread.hpp
  sal/thread.cpp
  sal/time.hpp
)


# unittests
list(APPEND sal_unittests
  sal/assert.test.cpp
  sal/builtins.test.cpp
  sal/char_array.test.cpp
  sal/common.test.hpp
  sal/error.test.cpp
  sal/file.test.cpp
  sal/format.test.cpp
  sal/intrusive_queue.test.cpp
  sal/lockable.test.cpp
  sal/memory_writer.test.cpp
  sal/queue.test.cpp
  sal/spinlock.test.cpp
  sal/thread.test.cpp
  sal/time.test.cpp
)


# submodules
include(sal/logger/list.cmake)
include(sal/net/list.cmake)
include(sal/program_options/list.cmake)
