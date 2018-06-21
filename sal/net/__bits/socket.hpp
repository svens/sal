#pragma once

#include <sal/config.hpp>
#include <sal/intrusive_queue.hpp>
#include <sal/intrusive_mpsc_queue.hpp>
#include <sal/type_id.hpp>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <system_error>

#if __sal_os_macos || __sal_os_linux // {{{1
  #include <sys/socket.h>
#elif __sal_os_windows // {{{1
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32")
#else // {{{1
  #error Unsupported platform
#endif // }}}1


__sal_begin


namespace net { namespace __bits {


const std::error_code &init_lib () noexcept;


#if __sal_os_windows // {{{1

#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH

using sa_family_t = ::ADDRESS_FAMILY;
using message_flags_t = DWORD;

#elif __sal_os_macos || __sal_os_linux // {{{1

using sa_family_t = ::sa_family_t;
using message_flags_t = int;

#endif // }}}1


enum class shutdown_t
{
  receive = SHUT_RD,
  send = SHUT_WR,
  both = SHUT_RDWR,
};


enum class wait_t
{
  read,
  write,
};


struct async_service_t;
using async_service_ptr = std::shared_ptr<async_service_t>;
struct async_context_t;
struct async_io_t;


struct socket_t
{
#if __sal_os_windows // {{{1
  using handle_t = SOCKET;
  static constexpr handle_t invalid = INVALID_SOCKET;
#elif __sal_os_macos || __sal_os_linux // {{{1
  using handle_t = int;
  static constexpr handle_t invalid = -1;
#endif // }}}1

  handle_t handle = invalid;

  socket_t () = default;

  socket_t (handle_t handle) noexcept
    : handle(handle)
  {}

  socket_t (socket_t &&that) noexcept
    : handle(that.handle)
  {
    that.handle = invalid;
  }

  socket_t &operator= (socket_t &&that) noexcept
  {
    auto tmp{std::move(that)};
    this->swap(tmp);
    return *this;
  }

  ~socket_t () noexcept
  {
    if (handle != invalid)
    {
      std::error_code ignored;
      close(ignored);
    }
  }

  socket_t (const socket_t &) = delete;
  socket_t &operator= (const socket_t &) = delete;

  void swap (socket_t &that) noexcept
  {
    using std::swap;
    swap(handle, that.handle);
  }

  void open (int domain,
    int type,
    int protocol,
    std::error_code &error
  ) noexcept;

  void close (std::error_code &error) noexcept;

  void bind (const void *address, size_t address_size, std::error_code &error)
    noexcept;

  void listen (int backlog, std::error_code &error) noexcept;

  handle_t accept (void *address, size_t *address_size,
    bool enable_connection_aborted,
    std::error_code &error
  ) noexcept;

  void connect (const void *address, size_t address_size,
    std::error_code &error
  ) noexcept;

  bool wait (wait_t what,
    int timeout_ms,
    std::error_code &error
  ) noexcept;

  void shutdown (shutdown_t what, std::error_code &error) noexcept;

  void remote_endpoint (void *address, size_t *address_size,
    std::error_code &error
  ) const noexcept;

  void local_endpoint (void *address, size_t *address_size,
    std::error_code &error
  ) const noexcept;

  void get_opt (int level, int name,
    void *data, size_t *size,
    std::error_code &error
  ) const noexcept;

  void set_opt (int level, int name,
    const void *data, size_t size,
    std::error_code &error
  ) noexcept;

  bool non_blocking (std::error_code &error) const noexcept;
  void non_blocking (bool mode, std::error_code &error) noexcept;

  size_t available (std::error_code &error) const noexcept;

  size_t receive (void *data, size_t data_size,
    message_flags_t flags,
    std::error_code &error
  ) noexcept;

  size_t receive_from (void *data, size_t data_size,
    void *address, size_t *address_size,
    message_flags_t flags,
    std::error_code &error
  ) noexcept;

  size_t send (const void *data, size_t data_size,
    message_flags_t flags,
    std::error_code &error
  ) noexcept;

  size_t send_to (const void *data, size_t data_size,
    const void *address, size_t address_size,
    message_flags_t flags,
    std::error_code &error
  ) noexcept;


  struct async_t;
  using async_ptr = std::unique_ptr<async_t>;
  async_ptr async{};

  void associate (async_service_ptr svc, std::error_code &error) noexcept;
};


struct async_io_base_t
{
#if __sal_os_windows // {{{1
  OVERLAPPED overlapped{0};
#endif // }}}1

  async_context_t * const owner, *context{};
  uintptr_t user_data{};

  uintptr_t op_id{};
  char op_data[160];
  std::error_code error{};

  char *begin{}, *end{};

  union
  {
    // any thread that finishes op pushes to owner's free list
    intrusive_mpsc_queue_hook_t<async_io_base_t> free{};

    // single thread pushes to completed but socket close can happen on any
    intrusive_mpsc_queue_hook_t<async_io_base_t> completed;

    // any thread can start receive
    // completing consumer side is synchronized externally
    intrusive_mpsc_queue_hook_t<async_io_base_t> pending_receive;

    // any thread can start send and complete
    // this queue is synchronized externally
    intrusive_queue_hook_t<async_io_base_t> pending_send;
  };

  using free_list = intrusive_mpsc_queue_t<
    async_io_base_t,
    &async_io_base_t::free
  >;
  using completed_list = intrusive_mpsc_queue_t<
    async_io_base_t,
    &async_io_base_t::completed
  >;
  using pending_receive_list = intrusive_mpsc_queue_t<
    async_io_base_t,
    &async_io_base_t::pending_receive
  >;
  using pending_send_list = intrusive_queue_t<
    async_io_base_t,
    &async_io_base_t::pending_send
  >;

  async_io_base_t (async_context_t *owner) noexcept
    : owner(owner)
  {}

  async_io_base_t (const async_io_base_t &) = delete;
  async_io_base_t (async_io_base_t &&) = delete;
  async_io_base_t &operator= (const async_io_base_t &) = delete;
  async_io_base_t &operator= (async_io_base_t &&) = delete;
};


struct async_io_t
  : public async_io_base_t
{
  static constexpr size_t data_size = 4096 - sizeof(async_io_base_t);
  char data[data_size];

  async_io_t (async_context_t *owner) noexcept
    : async_io_base_t(owner)
  {}

  static constexpr void static_checks () noexcept
  {
    static_assert(sizeof(async_io_t) == 4096);
    static_assert(data_size > 1500, "data_size less than MTU size");
  }
};


struct async_service_t
{
#if __sal_os_windows // {{{1
  HANDLE iocp;
#elif __sal_os_macos || __sal_os_linux // {{{1
  int queue;
#endif // }}}1

  static constexpr size_t max_events_per_poll = 256;

  async_service_t (std::error_code &error) noexcept;
  ~async_service_t () noexcept;

  async_service_t (const async_service_t &) = delete;
  async_service_t &operator= (const async_service_t &) = delete;
  async_service_t (async_service_t &&) = delete;
  async_service_t &operator= (async_service_t &&) = delete;
};


struct async_context_t
{
  async_service_ptr service;
  size_t max_events_per_poll;

  std::deque<std::unique_ptr<char[], void(*)(void*)>> pool{};
  async_io_t::free_list free{};
  async_io_t::completed_list completed{};


  async_context_t (async_service_ptr service, size_t max_events_per_poll)
    : service(service)
    , max_events_per_poll(max_events_per_poll)
  {
    if (max_events_per_poll < 1)
    {
      max_events_per_poll = 1;
    }
    else if (max_events_per_poll > async_service_t::max_events_per_poll)
    {
      max_events_per_poll = async_service_t::max_events_per_poll;
    }
  }


  async_context_t (async_context_t &&) = default;
  async_context_t &operator= (async_context_t &&) = default;

  async_context_t (const async_context_t &) = delete;
  async_context_t &operator= (const async_context_t &) = delete;

  static void *operator new (size_t) = delete;
  static void *operator new[] (size_t) = delete;

  void extend_pool ();


  async_io_t *new_io ()
  {
    auto io = static_cast<async_io_t *>(free.try_pop());
    if (!io)
    {
      extend_pool();
      io = static_cast<async_io_t *>(free.try_pop());
    }
    io->context = this;
    io->begin = io->data;
    io->end = io->data + sizeof(io->data);
    return io;
  }


  static void release_io (void *p) noexcept
  {
    auto io = reinterpret_cast<async_io_t *>(p);
    io->owner->free.push(io);
  }


  async_io_t *try_get () noexcept
  {
    return static_cast<async_io_t *>(completed.try_pop());
  }


  async_io_t *poll (const std::chrono::milliseconds &timeout,
    std::error_code &error
  ) noexcept;


#if __sal_os_windows // {{{1
  void enqueue_completions (OVERLAPPED_ENTRY *first, OVERLAPPED_ENTRY *last)
    noexcept;
#endif // }}}1
};


struct async_op_base_t
{
#if __sal_os_windows // {{{1
  DWORD transferred;
#elif __sal_os_macos || __sal_os_linux // {{{1
  size_t transferred;
#endif // }}}1
};


template <typename T>
struct async_op_t
  : public async_op_base_t
{
  static T *new_op (async_io_t *io) noexcept
  {
    static_assert(sizeof(T) <= sizeof(io->op_data));
    static_assert(std::is_trivially_destructible<T>());
    io->op_id = type_v<T>;
    return reinterpret_cast<T *>(io->op_data);
  }


  static T *get_op (async_io_t *io) noexcept
  {
    return io->op_id == type_v<T>
      ? reinterpret_cast<T *>(io->op_data)
      : nullptr;
  }


  static T *result (async_io_t *io, std::error_code &error) noexcept
  {
    if (io->op_id == type_v<T>)
    {
      error = io->error;
      return reinterpret_cast<T *>(io->op_data);
    }
    return nullptr;
  }
};


#if __sal_os_windows // {{{1


struct socket_t::async_t
{
  async_t (socket_t &socket, async_service_ptr service, std::error_code &error)
    noexcept;
};


struct async_receive_from_t
  : public async_op_t<async_receive_from_t>
{
  sockaddr_storage remote_address;
  INT remote_address_size;

  static void start (async_io_t *io,
    socket_t &socket,
    message_flags_t flags
  ) noexcept;
};


struct async_receive_t
  : public async_op_t<async_receive_t>
{
  static void start (async_io_t *io,
    socket_t &socket,
    message_flags_t flags
  ) noexcept;
};


struct async_send_to_t
  : public async_op_t<async_send_to_t>
{
  static void start (async_io_t *io,
    socket_t &socket,
    const void *address, size_t address_size,
    message_flags_t flags
  ) noexcept;
};


struct async_send_t
  : public async_op_t<async_send_t>
{
  static void start (async_io_t *io, socket_t &socket, message_flags_t flags)
    noexcept;
};


struct async_connect_t
  : public async_op_t<async_connect_t>
{
  socket_t::handle_t handle;

  static void start (async_io_t *io,
    socket_t &socket,
    const void *address, size_t address_size
  ) noexcept;

  static async_connect_t *result (async_io_t *io, std::error_code &error)
    noexcept;
};


struct async_accept_t
  : public async_op_t<async_accept_t>
{
  socket_t::handle_t accepted, acceptor;
  sockaddr_storage *remote_address;

  static void start (async_io_t *io, socket_t &socket, int family) noexcept;
  static async_accept_t *result (async_io_t *io, std::error_code &error) noexcept;

  socket_t::handle_t load_accepted () noexcept
  {
    auto result = accepted;
    accepted = socket_t::invalid;
    return result;
  }
};


#elif __sal_os_macos || __sal_os_linux // {{{1


struct socket_t::async_t
{
  async_service_ptr service;

  using mutex_t = std::mutex;
  using lock_t = std::lock_guard<mutex_t>;

  mutex_t receive_mutex{};
  async_io_t::pending_receive_list pending_receive{};

  mutex_t send_mutex{};
  async_io_t::pending_send_list pending_send{};
  bool listen_writable = false;

  async_t (socket_t &socket, async_service_ptr service, std::error_code &error)
    noexcept;

  ~async_t () noexcept;

  void push_send (socket_t &socket, async_io_t *io) noexcept;

  void on_readable (socket_t &socket,
    async_context_t &context,
    uint16_t flags
  ) noexcept;

  void on_writable (socket_t &socket,
    async_context_t &context,
    uint16_t flags
  ) noexcept;
};


struct async_receive_from_t
  : public async_op_t<async_receive_from_t>
{
  message_flags_t flags;
  sockaddr_storage remote_address;
  size_t remote_address_size;

  static void start (async_io_t *io,
    socket_t &socket,
    message_flags_t flags
  ) noexcept;
};


struct async_receive_t
  : public async_op_t<async_receive_t>
{
  message_flags_t flags;

  static void start (async_io_t *io,
    socket_t &socket,
    message_flags_t flags
  ) noexcept;
};


struct async_send_to_t
  : public async_op_t<async_send_to_t>
{
  sockaddr_storage address;
  size_t address_size;
  message_flags_t flags;

  static void start (async_io_t *io,
    socket_t &socket,
    const void *address, size_t address_size,
    message_flags_t flags
  ) noexcept;
};


struct async_send_t
  : public async_op_t<async_send_t>
{
  message_flags_t flags;

  static void start (async_io_t *io, socket_t &socket, message_flags_t flags)
    noexcept;
};


struct async_connect_t
  : public async_op_t<async_connect_t>
{
  static void start (async_io_t *io,
    socket_t &socket,
    const void *address, size_t address_size
  ) noexcept;
};


struct async_accept_t
  : public async_op_t<async_accept_t>
{
  socket_t::handle_t accepted;
  sockaddr_storage *local_address, *remote_address;

  static void start (async_io_t *io, socket_t &socket, int family) noexcept;

  socket_t::handle_t load_accepted () noexcept
  {
    auto result = accepted;
    accepted = socket_t::invalid;
    return result;
  }

  void load_local_address (std::error_code &error) noexcept;
};


#endif // }}}1


}} // namespace net::__bits


__sal_end
