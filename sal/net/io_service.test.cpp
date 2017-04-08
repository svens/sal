#include <sal/net/io_service.hpp>
#include <sal/net/internet.hpp>
#include <sal/common.test.hpp>


#if __sal_os_darwin
  #include <sal/thread.hpp>
  #include <sys/types.h>
  #include <sys/event.h>
  #include <sys/time.h>
  #include <forward_list>
  #include <thread>
  using namespace std::chrono_literals;
#endif


namespace {


struct net_io_service
  : public sal_test::fixture
{
  using datagram_socket_t = sal::net::ip::udp_t::socket_t;
  using stream_socket_t = sal::net::ip::tcp_t::socket_t;
  using acceptor_t = sal::net::ip::tcp_t::acceptor_t;

  sal::net::io_service_t service;
};


TEST_F(net_io_service, make_context)
{
  auto ctx = service.make_context();
  (void)ctx;
}


TEST_F(net_io_service, make_context_too_small_completion_count)
{
  auto ctx = service.make_context(0);
  (void)ctx;
}


TEST_F(net_io_service, make_context_too_big_completion_count)
{
  auto ctx = service.make_context((std::numeric_limits<size_t>::max)());
  (void)ctx;
}


TEST_F(net_io_service, associate_datagram_socket)
{
  datagram_socket_t socket(sal::net::ip::udp_t::v4());
  std::error_code error;
  service.associate(socket, error);
  EXPECT_TRUE(!error);
}


TEST_F(net_io_service, associate_datagram_socket_multiple_times)
{
#if !__sal_os_linux

  datagram_socket_t socket(sal::net::ip::udp_t::v4());

  std::error_code error;
  service.associate(socket, error);
  EXPECT_TRUE(!error);

  service.associate(socket, error);
  EXPECT_FALSE(!error);
  EXPECT_EQ(sal::net::socket_errc_t::already_associated, error);

  EXPECT_THROW(service.associate(socket), std::system_error);

#endif
}


TEST_F(net_io_service, associate_datagram_socket_invalid)
{
#if !__sal_os_linux

  datagram_socket_t socket;

  std::error_code error;
  service.associate(socket, error);
  EXPECT_FALSE(!error);

  EXPECT_THROW(service.associate(socket), std::system_error);

#endif
}


TEST_F(net_io_service, associate_stream_socket)
{
  stream_socket_t socket(sal::net::ip::tcp_t::v4());
  std::error_code error;
  service.associate(socket, error);
  EXPECT_TRUE(!error);
}


TEST_F(net_io_service, associate_stream_socket_multiple_times)
{
#if !__sal_os_linux

  stream_socket_t socket(sal::net::ip::tcp_t::v4());

  std::error_code error;
  service.associate(socket, error);
  EXPECT_TRUE(!error);

  service.associate(socket, error);
  EXPECT_FALSE(!error);
  EXPECT_EQ(sal::net::socket_errc_t::already_associated, error);

  EXPECT_THROW(service.associate(socket), std::system_error);

#endif
}


TEST_F(net_io_service, associate_stream_socket_invalid)
{
#if !__sal_os_linux

  stream_socket_t socket;

  std::error_code error;
  service.associate(socket, error);
  EXPECT_FALSE(!error);

  EXPECT_THROW(service.associate(socket), std::system_error);

#endif
}


TEST_F(net_io_service, associate_acceptor_socket)
{
  acceptor_t acceptor(sal::net::ip::tcp_t::v4());
  std::error_code error;
  service.associate(acceptor, error);
  EXPECT_TRUE(!error);
}


TEST_F(net_io_service, associate_acceptor_socket_multiple_times)
{
#if !__sal_os_linux

  acceptor_t acceptor(sal::net::ip::tcp_t::v4());

  std::error_code error;
  service.associate(acceptor, error);
  EXPECT_TRUE(!error);

  service.associate(acceptor, error);
  EXPECT_FALSE(!error);
  EXPECT_EQ(sal::net::socket_errc_t::already_associated, error);

  EXPECT_THROW(service.associate(acceptor), std::system_error);

#endif
}


TEST_F(net_io_service, associate_acceptor_socket_invalid)
{
#if !__sal_os_linux

  acceptor_t socket;

  std::error_code error;
  service.associate(socket, error);
  EXPECT_FALSE(!error);

  EXPECT_THROW(service.associate(socket), std::system_error);

#endif
}


#if __sal_os_darwin


using namespace std::chrono;
using namespace std::chrono_literals;


struct kqueue
  : public sal_test::fixture
{
  int queue;

  using socket_t = sal::net::ip::udp_t::socket_t;
  const socket_t::endpoint_t endpoint{sal::net::ip::address_v4_t::loopback(), 8192};
  socket_t a{endpoint}, b{sal::net::ip::udp_t::v4()};

  void SetUp ()
  {
    queue = ::kqueue();
    if (queue == -1)
    {
      FAIL() << strerror(errno);
    }

    a.non_blocking(true);
  }

  void TearDown ()
  {
    if (queue != -1)
    {
      ::close(queue);
    }
  }

  void send (const std::string &data, bool wait_after_send = true)
  {
    b.send_to(sal::make_buf(data), endpoint);
    if (wait_after_send)
    {
      std::this_thread::sleep_for(1ms);
    }
  }

  std::string recv (std::error_code &error)
  {
    char buf[1024];
    socket_t::endpoint_t remote_endpoint;
    auto size = a.receive_from(sal::make_buf(buf), remote_endpoint, error);
    return std::string(buf, size);
  }

  std::string recv ()
  {
    return recv(sal::net::throw_on_error("recv"));
  }

  using string_list = std::vector<std::string>;

  string_list drain (std::error_code &error)
  {
    string_list result;
    for (;;)
    {
      auto packet = recv(error);
      if (!error)
      {
        result.emplace_back(packet);
      }
      else
      {
        if (error == std::errc::operation_would_block)
        {
          error.clear();
        }
        break;
      }
    }
    return result;
  }

  string_list drain ()
  {
    return drain(sal::net::throw_on_error("drain"));
  }

  void monitor (int filter, int flags)
  {
    struct ::kevent change;
    EV_SET(&change, a.native_handle(), filter, flags, 0, 0, 0);
    if (::kevent(queue, &change, 1, nullptr, 0, nullptr) == -1)
    {
      FAIL() << strerror(errno);
    }
  }

  using event_list = std::vector<struct ::kevent>;

  event_list poll (const milliseconds &timeout, std::error_code &error)
  {
    auto s = duration_cast<seconds>(timeout);
    auto ns = duration_cast<nanoseconds>(timeout - s);
    struct timespec t = { s.count(), ns.count() };

    struct ::kevent events[16];
    auto count = ::kevent(queue, nullptr, 0, events, 16, &t);
    if (count == -1)
    {
      error.assign(errno, std::generic_category());
      return {};
    }

    event_list result;
    for (auto i = 0;  i != count;  ++i)
    {
      result.emplace_back(events[i]);
    }
    return result;
  }

  event_list poll (const milliseconds &timeout = 3s)
  {
    return poll(timeout, sal::net::throw_on_error("poll"));
  }

  sal::net::socket_base_t::native_handle_t handle (struct ::kevent &ev)
    const noexcept
  {
    return ev.ident;
  }

  size_t data_size (struct ::kevent &ev) const noexcept
  {
    return ev.data;
  }
};


TEST_F(kqueue, empty)
{
  monitor(EVFILT_READ, EV_ADD | EV_CLEAR);
  EXPECT_EQ(0U, poll(0ms).size());
}


TEST_F(kqueue, basic)
{
  monitor(EVFILT_READ, EV_ADD | EV_CLEAR);

  send(case_name);
  auto events = poll();
  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(a.native_handle(), handle(events[0]));
  EXPECT_EQ(case_name.size(), data_size(events[0]));
  EXPECT_EQ(case_name, recv());
}


TEST_F(kqueue, edge_triggered_no_second_notification)
{
  monitor(EVFILT_READ, EV_ADD | EV_CLEAR);

  send("one");
  auto events = poll();
  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(3U, data_size(events[0]));

  events = poll(0ms);
  EXPECT_EQ(0U, events.size());

  EXPECT_EQ("one", recv());
}


TEST_F(kqueue, edge_triggered_second_notification_on_new_send)
{
  monitor(EVFILT_READ, EV_ADD | EV_CLEAR);

  send("one");
  auto events = poll();
  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(3U, data_size(events[0]));

  send("two");
  events = poll(0ms);
  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(6U, data_size(events[0]));

  string_list packets{"one", "two"};
  EXPECT_EQ(packets, drain());
}


TEST_F(kqueue, level_triggered_second_notification)
{
  monitor(EVFILT_READ, EV_ADD);

  send("one");
  auto events = poll();
  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(3U, data_size(events[0]));

  events = poll(0ms);
  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(3U, data_size(events[0]));

  EXPECT_EQ("one", recv());
}


TEST_F(kqueue, level_triggered_second_notification_on_new_send)
{
  monitor(EVFILT_READ, EV_ADD);

  send("one");
  auto events = poll();
  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(3U, data_size(events[0]));

  send("two");
  events = poll(0ms);
  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(6U, data_size(events[0]));

  string_list packets{"one", "two"};
  EXPECT_EQ(packets, drain());
}


TEST_F(kqueue, single_thread_wakeup)
{
  monitor(EVFILT_READ, EV_ADD | EV_CLEAR);

  send("one", false);
  send("two");

  std::atomic<size_t> event_count{};
  std::forward_list<std::thread> threads;
  for (auto i = 0U;  i < std::thread::hardware_concurrency();  ++i)
  {
    threads.emplace_front([&event_count, this]
    {
      auto events = poll(1ms);
      event_count += events.size();
      if (events.size())
      {
        EXPECT_EQ(6U, data_size(events[0]));
        EXPECT_EQ("one", recv());
        EXPECT_EQ("two", recv());
      }
    });
  }

  for (auto &thread: threads)
  {
    thread.join();
  }

  EXPECT_EQ(1U, event_count);
}


#endif // __sal_os_darwin


} // namespace
