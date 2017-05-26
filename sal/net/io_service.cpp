#include <sal/net/io_service.hpp>


__sal_begin


namespace net {


void io_service_t::associate (__bits::socket_t &socket, std::error_code &error)
  noexcept
{
  if (socket.async)
  {
    error = make_error_code(socket_errc::already_associated);
    return;
  }

  socket.async.reset(
    new(std::nothrow) __bits::socket_t::async_t(socket, impl_, error)
  );

  if (!socket.async)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  else if (error)
  {
    socket.async.reset();
  }
  else
  {
    error.clear();
  }
}


char *alloc_batch (size_t batch_size)
{
  return new char[batch_size];
}


void free_batch (char *p)
{
  delete[] p;
}


void io_service_t::context_t::extend_pool ()
{
  constexpr auto batch_size = 1024 * sizeof(buf_t);
  pool_.emplace_back(alloc_batch(batch_size), &free_batch);

  auto *it = pool_.back().get(), * const e = it + batch_size;
  for (/**/;  it != e;  it += sizeof(buf_t))
  {
    // see "Ugly hack" in buf_t private section
    free_.push(reinterpret_cast<__bits::io_buf_t *>(new(it) buf_t(this)));
  }
}


} // namespace net


__sal_end
