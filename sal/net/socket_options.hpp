#pragma once

/**
 * \file sal/net/socket_options.hpp
 * Socket option getters and setters
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <chrono>


__sal_begin


namespace net {


namespace __bits {


template <int Level, int Name, typename Type>
struct socket_option_setter_t
{
  static constexpr int level = Level;
  static constexpr int name = Name;

  using native_t = int;
  Type data;

  socket_option_setter_t (Type value) noexcept
    : data(value)
  {}

  void store (native_t &value) const noexcept
  {
    value = data;
  }
};


inline void assign (bool &dest, int source) noexcept
{
  dest = !!source;
}


inline void assign (int &dest, int source) noexcept
{
  dest = source;
}


template <int Level, int Name, typename Type>
struct socket_option_getter_t
{
  static constexpr int level = Level;
  static constexpr int name = Name;

  using native_t = int;
  Type *data;

  socket_option_getter_t (Type *value) noexcept
    : data(value)
  {}

  void load (const native_t &value, size_t) const noexcept
  {
    assign(*data, value);
  }
};


} // namespace __bits


//
// reuse_address
//

auto reuse_address (bool value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_REUSEADDR, bool>
{
  return value;
}


auto reuse_address (bool *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_REUSEADDR, bool>
{
  return value;
}


//
// receive_buffer_size
//

auto receive_buffer_size (int value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_RCVBUF, int>
{
  return value;
}


auto receive_buffer_size (int *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_RCVBUF, int>
{
  return value;
}


//
// linger
//

namespace __bits {


struct socket_option_linger_setter_t
{
  static constexpr int level = SOL_SOCKET;
  static constexpr int name = SO_LINGER;

  using native_t = ::linger;
  bool on;
  std::chrono::seconds timeout;

  socket_option_linger_setter_t (bool on, std::chrono::seconds timeout) noexcept
    : on(on)
    , timeout(timeout)
  {}

  void store (native_t &value) const noexcept
  {
    value.l_onoff = on;
    value.l_linger = static_cast<decltype(value.l_linger)>(timeout.count());
  }
};


struct socket_option_linger_getter_t
{
  static constexpr int level = SOL_SOCKET;
  static constexpr int name = SO_LINGER;

  using native_t = ::linger;
  bool *on;
  std::chrono::seconds *timeout;

  socket_option_linger_getter_t (bool *on, std::chrono::seconds *timeout)
    noexcept
    : on(on)
    , timeout(timeout)
  {}

  void load (const native_t &value, size_t) const noexcept
  {
    *on = !!value.l_onoff;
    *timeout = std::chrono::seconds(value.l_linger);
  }
};


} // namespace __bits


auto linger (bool on, std::chrono::seconds timeout) noexcept
  -> __bits::socket_option_linger_setter_t
{
  return __bits::socket_option_linger_setter_t{ on, timeout };
}


auto linger (bool *on, std::chrono::seconds *timeout) noexcept
  -> __bits::socket_option_linger_getter_t
{
  return __bits::socket_option_linger_getter_t{ on, timeout };
}


} // namespace net


__sal_end
