#pragma once

/**
 * \file sal/net/async/io.hpp
 * Asynchronous network I/O operation
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/assert.hpp>
#include <sal/type_id.hpp>


__sal_begin


namespace net::async {


class io_t
{
public:

  void reset () noexcept
  {
    impl_->reset();
  }


  explicit operator bool () const noexcept
  {
    return impl_.get() != nullptr;
  }


  template <typename Context>
  void context (Context *context) noexcept
  {
    impl_->context = context;
    impl_->context_type = type_v<Context>;
  }


  template <typename Context>
  Context *context () const noexcept
  {
    if (impl_->context_type == type_v<Context>)
    {
      return static_cast<Context *>(impl_->context);
    }
    return nullptr;
  }


  template <typename Context>
  Context *socket_context () const
  {
    auto &current_owner = *sal_check_ptr(impl_->current_owner);
    if (current_owner.context_type == type_v<Context>)
    {
      return static_cast<Context *>(current_owner.context);
    }
    return nullptr;
  }


  const uint8_t *head () const noexcept
  {
    return impl_->data;
  }


  const uint8_t *tail () const noexcept
  {
    return impl_->data + sizeof(impl_->data);
  }


  uint8_t *data () noexcept
  {
    return impl_->begin;
  }


  uint8_t *begin () noexcept
  {
    return impl_->begin;
  }


  const uint8_t *end () const noexcept
  {
    return impl_->end;
  };


  void head_gap (size_t offset_from_head)
  {
    sal_assert(offset_from_head <= max_size());
    impl_->begin = impl_->data + offset_from_head;
  }


  size_t head_gap () const noexcept
  {
    return impl_->begin - impl_->data;
  }


  void tail_gap (size_t offset_from_tail)
  {
    sal_assert(offset_from_tail <= max_size());
    impl_->end = impl_->data + sizeof(impl_->data) - offset_from_tail;
  }


  size_t tail_gap () const noexcept
  {
    return impl_->data + sizeof(impl_->data) - impl_->end;
  }


  size_t size () const noexcept
  {
    return impl_->end - impl_->begin;
  }


  void resize (size_t new_size)
  {
    sal_assert(impl_->begin + new_size <= impl_->data + sizeof(impl_->data));
    impl_->end = impl_->begin + new_size;
  }


  static constexpr size_t max_size () noexcept
  {
    return sizeof(impl_->data);
  }


  template <typename Result>
  const Result *get_if (std::error_code &error) const noexcept
  {
    if (impl_->result_type == type_v<Result>)
    {
      error = impl_->status;
      return reinterpret_cast<const Result *>(impl_->result_data);
    }
    return nullptr;
  }


  template <typename Result>
  const Result *get_if () const
  {
    return get_if<Result>(throw_on_error("async::io::get_if"));
  }


private:

  __bits::io_ptr impl_;

  io_t (__bits::io_t *impl) noexcept
    : impl_(impl)
  { }


  template <typename Result>
  __bits::io_t *to_async_op (Result **result) noexcept
  {
    auto op = impl_.release();
    static_assert(std::is_trivially_destructible_v<Result>);
    static_assert(sizeof(Result) <= sizeof(op->result_data));
    op->result_type = type_v<Result>;
    *result = reinterpret_cast<Result *>(op->result_data);
    return op;
  }


  friend class service_t;
  friend class worker_t;
  template <typename Protocol> friend class net::basic_datagram_socket_t;
};


} // namespace net::async


__sal_end
