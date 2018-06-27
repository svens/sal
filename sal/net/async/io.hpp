#pragma once

/**
 * \file sal/net/async/service.hpp
 * Asynchronous networking service
 */


#include <sal/config.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/error.hpp>


__sal_begin


namespace net::async {


class io_t
{
public:

  void reset () noexcept
  {
    impl_->begin = impl_->data;
    impl_->end = impl_->data + sizeof(impl_->data);
    impl_->user_data = nullptr;
  }


  void release () noexcept
  {
    impl_.reset();
  }


  explicit operator bool () const noexcept
  {
    return impl_.get() != nullptr;
  }


  template <typename UserData>
  void user_data (UserData *ptr) noexcept
  {
    impl_->user_data = ptr;
  }


  void *user_data () noexcept
  {
    return impl_->user_data;
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
    sal_throw_if(offset_from_head > max_size());
    impl_->begin = impl_->data + offset_from_head;
  }


  size_t head_gap () const noexcept
  {
    return impl_->begin - impl_->data;
  }


  void tail_gap (size_t offset_from_tail)
  {
    sal_throw_if(offset_from_tail > max_size());
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
    sal_throw_if(impl_->begin + new_size > impl_->data + sizeof(impl_->data));
    impl_->end = impl_->begin + new_size;
  }


  static constexpr size_t max_size () noexcept
  {
    return sizeof(impl_->data);
  }


#if 0
  template <typename AsyncResult>
  const AsyncResult *result_for (std::error_code &error) noexcept
  {
    error.clear();
    return nullptr;
  }


  template <typename AsyncResult>
  const AsyncResult *result_for ()
  {
    return result_for<AsyncResult>(throw_on_error("io::result_for"));
  }
#endif


private:

  __bits::io_ptr impl_;

  io_t (__bits::io_t *impl) noexcept
    : impl_(impl)
  { }

  friend class service_t;
};


} // namespace net::async


__sal_end
