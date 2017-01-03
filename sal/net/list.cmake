# docs
list(APPEND sal_docs
)


# sources
list(APPEND sal_sources
  sal/net/__bits/platform.hpp
  sal/net/__bits/platform.cpp
  sal/net/fwd.hpp

  sal/net/basic_socket.hpp
  sal/net/basic_datagram_socket.hpp
  sal/net/basic_stream_socket.hpp
  sal/net/basic_socket_acceptor.hpp
  sal/net/error.hpp
  sal/net/error.cpp
  sal/net/socket.hpp
  sal/net/socket_base.hpp
  sal/net/socket_options.hpp

  sal/net/internet.hpp
  sal/net/ip/address.hpp
  sal/net/ip/address_v4.hpp
  sal/net/ip/address_v6.hpp
  sal/net/ip/basic_endpoint.hpp
  sal/net/ip/basic_resolver.hpp
  sal/net/ip/basic_resolver_entry.hpp
  sal/net/ip/basic_resolver_results.hpp
  sal/net/ip/basic_resolver_results_iterator.hpp
  sal/net/ip/resolver.hpp
  sal/net/ip/resolver_base.hpp
  sal/net/ip/tcp.hpp
  sal/net/ip/udp.hpp
)


# unittests
list(APPEND sal_unittests
  sal/net/platform.test.cpp

  sal/net/error.test.cpp
  sal/net/socket.test.cpp

  sal/net/ip/address.test.cpp
  sal/net/ip/address_v4.test.cpp
  sal/net/ip/address_v6.test.cpp
  sal/net/ip/datagram_socket.test.cpp
  sal/net/ip/endpoint.test.cpp
  sal/net/ip/resolver.test.cpp
  sal/net/ip/socket_acceptor.test.cpp
  sal/net/ip/stream_socket.test.cpp
)
