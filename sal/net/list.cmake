# docs
list(APPEND sal_docs
)


# sources
list(APPEND sal_sources
  sal/net/__bits/platform.hpp
  sal/net/__bits/platform.cpp
  sal/net/fwd.hpp
  sal/net/ip/address.hpp
)


# unittests
list(APPEND sal_unittests
  sal/net/platform.test.cpp
  sal/net/ip/address.test.cpp
)
