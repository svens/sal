#include <bench/bench.hpp>
#include <sal/net/async/service.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/lockable.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <unordered_map>


using namespace std::chrono_literals;


namespace {


auto address = sal::net::ip::make_address_v4("0.0.0.0");
size_t worker_count = std::thread::hardware_concurrency();
size_t receive_count = 10'000;

using protocol_t = sal::net::ip::udp_t;
using socket_t = protocol_t::socket_t;


struct service_t
{
  service_t ()
    : client({address, 3478})
    , peer({address, 3479})
    , thread_statistics(worker_count)
  {
    client.associate(service);
    client.context(&client);

    peer.associate(service);
    peer.context(&peer);
  }

  //
  // sessions
  //

  using session_map = std::unordered_map<uint64_t, protocol_t::endpoint_t>;
  session_map sessions_data{10'000};
  sal::lockable_t<session_map> sessions{sessions_data};


  //
  // I/O
  //

  sal::net::async::service_t service{};
  socket_t client, peer;

  struct statistics_t
  {
    using list_t = std::vector<statistics_t>;
    size_t client_send{};
    size_t peer_recv{};

    statistics_t &operator<< (statistics_t &that) noexcept
    {
      client_send += that.client_send;
      that.client_send = 0;

      peer_recv += that.peer_recv;
      that.peer_recv = 0;

      return *this;
    }
  };
  statistics_t::list_t thread_statistics;
  statistics_t total{};

  void print_statistics ();
  void run (size_t thread_index);


  void start ()
  {
    for (auto i = receive_count;  i;  --i)
    {
      client.start_receive_from(service.make_io());
      peer.start_receive_from(service.make_io());
    }
  }


  void on_client_recv (sal::net::async::io_t &&io,
    const socket_t::receive_from_t *event,
    statistics_t &)
  {
    if (event->transferred == sizeof(session_map::key_type))
    {
      sessions.lock()->try_emplace(
        *reinterpret_cast<const session_map::key_type *>(io.data()),
        event->remote_endpoint
      );
    }
    client.start_receive_from(std::move(io));
  }


  void on_peer_recv (sal::net::async::io_t &&io,
    const socket_t::receive_from_t *event,
    statistics_t &statistics)
  {
    auto it = sessions.lock()->find(
      *reinterpret_cast<const session_map::key_type *>(io.data())
    );
    if (it != sessions_data.end())
    {
      ++statistics.peer_recv;
      io.resize(event->transferred);
      client.start_send_to(std::move(io), it->second);
    }
    else
    {
      peer.start_receive_from(std::move(io));
    }
  }
};


void service_t::run (size_t thread_index)
{
  auto &statistics = thread_statistics[thread_index];

  for (;;)
  {
    if (auto io = service.wait())
    {
      if (auto recv = io.get_if<socket_t::receive_from_t>())
      {
        if (io.socket_context<socket_t>() == &peer)
        {
          on_peer_recv(std::move(io), recv, statistics);
        }
        else
        {
          on_client_recv(std::move(io), recv, statistics);
        }
      }
      else
      {
        if (io.socket_context<socket_t>() == &client)
        {
          io.reset();
          peer.start_receive_from(std::move(io));
          ++statistics.client_send;
        }
      }
    }
  }
}


void service_t::print_statistics ()
{
  size_t sessions_count = sessions.lock()->size();

  statistics_t sum;

  for (auto &statistics: thread_statistics)
  {
    sum << statistics;
  }

  std::string sess = "sess: ";
  sess += std::to_string(sessions_count);

  std::string recv = "recv: ";
  recv += std::to_string(sum.peer_recv);

  std::string send = "send: ";
  send += std::to_string(sum.client_send);

  std::string diff = "diff: ";
  diff += std::to_string(static_cast<int>(sum.peer_recv - sum.client_send));

  std::cout
    << std::setw(20) << std::left << sess
    << std::setw(20) << std::left << recv
    << std::setw(20) << std::left << send
    << std::setw(20) << std::left << diff
    << '\n';
}


} // namespace


namespace bench {


option_set_t options ()
{
  using namespace sal::program_options;

  option_set_t desc;
  desc
    .add({"a", "address"},
      requires_argument("ADDRESS", address),
      help("UDP echo server IPv4 address")
    )
    .add({"r", "receives"},
      requires_argument("INT", receive_count),
      help("max outstanding receives")
    )
    .add({"w", "workers"},
      requires_argument("INT", worker_count),
      help("number of workers")
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  std::cout << "    address: ";
  address = sal::net::ip::make_address_v4(
    options.back_or_default("address", {arguments})
  );
  std::cout << address << '\n';

  std::cout << "   receives: ";
  receive_count = std::stoul(
    options.back_or_default("receives", {arguments})
  );
  std::cout << receive_count << '\n';

  std::cout << "    workers: ";
  worker_count = std::stoul(
    options.back_or_default("workers", {arguments})
  );
  std::cout << worker_count << '\n';

  service_t service;

  std::vector<std::thread> threads;
  while (threads.size() < worker_count)
  {
    auto index = threads.size();
    threads.emplace_back(&service_t::run, &service, index);
  }

  service.start();

  std::cout << '\n';
  for (;;)
  {
    std::this_thread::sleep_for(2s);
    service.print_statistics();
  }

  return EXIT_SUCCESS;
}


} // namespace bench
