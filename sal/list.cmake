# sources
list(APPEND sal_sources
  sal/__bits/member_assign.hpp
  sal/array_string.hpp
  sal/assert.hpp
  sal/builtins.hpp
  sal/error.hpp
  sal/file.hpp
  sal/file.cpp
  sal/fmt.hpp
  sal/__bits/fmt.hpp
  sal/intrusive_queue.hpp
  sal/__bits/intrusive_queue.hpp
  sal/spinlock.hpp
  sal/thread.hpp
  sal/thread.cpp
  sal/time.hpp
)


# unittests
list(APPEND sal_unittests
  sal/array_string.test.cpp
  sal/assert.test.cpp
  sal/builtins.test.cpp
  sal/common.test.hpp
  sal/error.test.cpp
  sal/file.test.cpp
  sal/fmt.test.cpp
  sal/intrusive_queue.test.cpp
  sal/spinlock.test.cpp
  sal/thread.test.cpp
  sal/time.test.cpp
)


# submodules
include(sal/logger/list.cmake)
include(sal/program_options/list.cmake)
