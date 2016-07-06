# sources
list(APPEND sal_sources
  sal/logger/fwd.hpp
  sal/logger/event.hpp
  sal/logger/level.hpp
  sal/logger/logger.hpp
  sal/logger/sink.hpp
)


# unittests
list(APPEND sal_unittests
  sal/logger/common.test.hpp
  sal/logger/level.test.cpp
  sal/logger/logger.test.cpp
)
