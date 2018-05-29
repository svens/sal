# sources
list(APPEND sal_sources
  sal/service/application.hpp
  sal/service/application.cpp
  sal/service/service_base.hpp
  sal/service/service_base.cpp
)


# unittests
list(APPEND sal_unittests_sources
  sal/service/application.test.cpp
  sal/service/service_base.test.cpp
)
