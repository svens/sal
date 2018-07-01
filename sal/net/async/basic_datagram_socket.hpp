#pragma once

/**
 * \file sal/net/async/basic_datagram_socket.hpp
 */


#include <sal/config.hpp>
#include <sal/net/async/basic_socket.hpp>


__sal_begin


namespace net::async {


template <typename Protocol>
class basic_datagram_socket_t
  : public basic_socket_t<Protocol>
{
  using base_t = basic_socket_t<Protocol>;

public:

  using handle_t = typename base_t::handle_t;
  using protocol_t = typename base_t::protocol_t;
  using endpoint_t = typename base_t::endpoint_t;


  basic_datagram_socket_t () = default;


  basic_datagram_socket_t (const Protocol &protocol)
    : base_t(protocol)
  {}


  basic_datagram_socket_t (const typename Protocol::endpoint_t &endpoint)
    : base_t(endpoint)
  {}


  basic_datagram_socket_t (handle_t handle)
    : base_t(handle)
  {}


  struct receive_from_t
  {
    size_t transferred;
    endpoint_t remote_endpoint;
  };


  void start_receive_from (io_t &&io, socket_base_t::message_flags_t flags)
    noexcept
  {
    auto result = base_t::template result_storage<receive_from_t>(io);
    base_t::impl_->start_receive_from(
      base_t::acquire(std::move(io)),
      result->remote_endpoint.data(),
      result->remote_endpoint.capacity(),
      &result->transferred,
      flags
    );
  }


  void start_receive_from (io_t &&io)
  {
    start_receive_from(std::move(io), socket_base_t::message_flags_t{});
  }


  static const receive_from_t *receive_from_result (const io_t &io,
    std::error_code &error) noexcept
  {
    return base_t::template result_of<receive_from_t>(io, error);
  }


  static const receive_from_t *receive_from_result (const io_t &io)
  {
    return receive_from_result(io,
      throw_on_error("basic_datagram_socket::receive_from_result")
    );
  }


  struct send_to_t
  {
    size_t transferred;
    endpoint_t remote_endpoint;
  };


  void start_send_to (io_t &&io,
    const endpoint_t &endpoint,
    socket_base_t::message_flags_t flags) noexcept
  {
    auto result = base_t::template result_storage<send_to_t>(io);
    result->remote_endpoint = endpoint;
    base_t::impl_->start_send_to(
      base_t::acquire(std::move(io)),
      result->remote_endpoint.data(),
      result->remote_endpoint.capacity(),
      &result->transferred,
      flags
    );
  }


  void start_send_to (io_t &&io, const endpoint_t &endpoint) noexcept
  {
    start_send_to(std::move(io), endpoint, socket_base_t::message_flags_t{});
  }


  static const send_to_t *send_to_result (const io_t &io,
    std::error_code &error) noexcept
  {
    return base_t::template result_of<send_to_t>(io, error);
  }


  static const send_to_t *send_to_result (const io_t &io)
  {
    return base_t::template result_of<send_to_t>(io,
      throw_on_error("basic_datagram_socket::send_to_result")
    );
  }
};


template <typename Endpoint>
basic_datagram_socket_t (const Endpoint &)
  -> basic_datagram_socket_t<typename Endpoint::protocol_t>;


} // namespace net::async


__sal_end
