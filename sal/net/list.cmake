# docs
list(APPEND sal_docs
)


# sources
list(APPEND sal_sources
  sal/net/__bits/platform.hpp
  sal/net/__bits/platform.cpp
  sal/net/ip/address_v4.hpp
)


# unittests
list(APPEND sal_unittests
  sal/net/ip/address_v4.test.cpp
)
