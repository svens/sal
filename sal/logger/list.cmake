# sources
list(APPEND sal_sources
  sal/logger/__bits/channel.hpp
  sal/logger/__bits/channel.cpp
  sal/logger/async_worker.hpp
  sal/logger/async_worker.cpp
  sal/logger/channel.hpp
  sal/logger/event.hpp
  sal/logger/fwd.hpp
  sal/logger/logger.hpp
  sal/logger/sink.hpp
  sal/logger/sink.cpp
  sal/logger/worker.hpp
  sal/logger/worker.cpp
)


# unittests
list(APPEND sal_unittests
  sal/logger/common.test.hpp
  sal/logger/channel.test.cpp
  sal/logger/logger.test.cpp
  sal/logger/worker.test.cpp
)
