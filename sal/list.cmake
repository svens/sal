# sources
list(APPEND sal_sources
  sal/__bits/member_assign.hpp
  sal/__bits/platform_sdk.hpp
  sal/__bits/ref.hpp
  sal/assert.hpp
  sal/buf_ptr.hpp
  sal/builtins.hpp
  sal/byte_order.hpp
  sal/char_array.hpp
  sal/__bits/base64.hpp
  sal/__bits/base64.cpp
  sal/__bits/hex.hpp
  sal/__bits/hex.cpp
  sal/encode.hpp
  sal/error.hpp
  sal/file.hpp
  sal/file.cpp
  sal/format.hpp
  sal/__bits/format.hpp
  sal/hash.hpp
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
  sal/type_id.hpp
)


# unittests
list(APPEND sal_unittests
  sal/assert.test.cpp
  sal/buf_ptr.test.cpp
  sal/builtins.test.cpp
  sal/byte_order.test.cpp
  sal/char_array.test.cpp
  sal/common.test.hpp
  sal/encode.test.cpp
  sal/error.test.cpp
  sal/file.test.cpp
  sal/format.test.cpp
  sal/hash.test.cpp
  sal/intrusive_queue.test.cpp
  sal/lockable.test.cpp
  sal/memory_writer.test.cpp
  sal/queue.test.cpp
  sal/spinlock.test.cpp
  sal/thread.test.cpp
  sal/time.test.cpp
  sal/type_id.test.cpp
  sal/type_id.unit_a.test.cpp
  sal/type_id.unit_b.test.cpp
)


# submodules
include(sal/crypto/list.cmake)
include(sal/logger/list.cmake)
include(sal/net/list.cmake)
include(sal/program_options/list.cmake)
