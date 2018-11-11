#pragma once

#include <sal/net/internet.hpp>
#include <sal/common.test.hpp>


namespace sal_test {


using address_types = ::testing::Types<
  sal::net::ip::address_v4_t,
  sal::net::ip::address_v6_t
>;


struct address_names
{
  template <typename T>
  static std::string GetName (int i)
  {
    if constexpr (std::is_same_v<T, sal::net::ip::address_v4_t>)
    {
      return "ipv4";
    }
    else if constexpr (std::is_same_v<T, sal::net::ip::address_v6_t>)
    {
      return "ipv6";
    }
    return std::to_string(i);
  }
};


using protocol_types = testing::Types<
  sal::net::ip::tcp_t,
  sal::net::ip::udp_t
>;


struct protocol_names
{
  template <typename T>
  static std::string GetName (int i)
  {
    if constexpr (std::is_same_v<T, sal::net::ip::tcp_t>)
    {
      return "tcp";
    }
    else if constexpr (std::is_same_v<T, sal::net::ip::udp_t>)
    {
      return "udp";
    }
    return std::to_string(i);
  }
};


using socket_types = ::testing::Types<
  sal::net::ip::udp_t::socket_t,
  sal::net::ip::tcp_t::socket_t,
  sal::net::ip::tcp_t::acceptor_t
>;


struct socket_names
{
  template <typename T>
  static std::string GetName (int i)
  {
    if constexpr (std::is_same_v<T, sal::net::ip::udp_t::socket_t>)
    {
      return "udp_socket";
    }
    else if constexpr (std::is_same_v<T, sal::net::ip::tcp_t::socket_t>)
    {
      return "tcp_socket";
    }
    else if constexpr (std::is_same_v<T, sal::net::ip::tcp_t::acceptor_t>)
    {
      return "tcp_acceptor";
    }
    return std::to_string(i);
  }
};


} // namespace sal_test
