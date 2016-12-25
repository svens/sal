# docs
list(APPEND sal_docs
)


# sources
list(APPEND sal_sources
  sal/net/__bits/platform.hpp
  sal/net/__bits/platform.cpp
  sal/net/error.hpp
  sal/net/error.cpp
  sal/net/fwd.hpp
  sal/net/ip/address.hpp
  sal/net/ip/address_v4.hpp
  sal/net/ip/address_v6.hpp
  sal/net/ip/endpoint.hpp
  sal/net/ip/tcp.hpp
  sal/net/ip/udp.hpp
)


# unittests
list(APPEND sal_unittests
  sal/net/error.test.cpp
  sal/net/platform.test.cpp
  sal/net/ip/address.test.cpp
  sal/net/ip/address_v4.test.cpp
  sal/net/ip/address_v6.test.cpp
  sal/net/ip/endpoint.test.cpp
)
