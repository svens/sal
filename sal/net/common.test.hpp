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
  static std::string GetName (int)
  {
    if constexpr (std::is_same_v<T, sal::net::ip::address_v4_t>)
    {
      return "v4";
    }
    else if constexpr (std::is_same_v<T, sal::net::ip::address_v6_t>)
    {
      return "v6";
    }
  }
};


using protocol_types = testing::Types<
  sal::net::ip::tcp_t,
  sal::net::ip::udp_t
>;


struct protocol_names
{
  template <typename T>
  static std::string GetName (int)
  {
    if constexpr (std::is_same_v<T, sal::net::ip::tcp_t>)
    {
      return "TCP";
    }
    else if constexpr (std::is_same_v<T, sal::net::ip::udp_t>)
    {
      return "UDP";
    }
  }
};


using tcp_v4_t = std::pair<sal::net::ip::tcp_t, sal::net::ip::address_v4_t>;
using tcp_v6_t = std::pair<sal::net::ip::tcp_t, sal::net::ip::address_v6_t>;
using udp_v4_t = std::pair<sal::net::ip::udp_t, sal::net::ip::address_v4_t>;
using udp_v6_t = std::pair<sal::net::ip::udp_t, sal::net::ip::address_v6_t>;


using protocol_and_address_types = testing::Types<
  tcp_v4_t, tcp_v6_t, udp_v4_t, udp_v6_t
>;


struct protocol_and_address_names
{
  template <typename T>
  static std::string GetName (int i)
  {
    return
      protocol_names::GetName<typename T::first_type>(i) +
      address_names::GetName<typename T::second_type>(i)
    ;
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
  static std::string GetName (int)
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
  }
};


} // namespace sal_test
