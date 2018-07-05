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
size_t poll_result_count = 100;
size_t completions_count = 200'000;
size_t receive_count = 50'000;
size_t send_count = 50'000;

using protocol_t = sal::net::ip::udp_t;
using socket_t = protocol_t::async_socket_t;


struct service_t
{
  service_t ()
    : async(completions_count)
    , client({address, 3478})
    , peer({address, 3479})
    , thread_statistics(worker_count)
  {
    client.associate(async, receive_count, send_count);
    client.context(&client);

    peer.associate(async, receive_count, send_count);
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

  sal::net::async::service_t async;
  socket_t client, peer;

  struct statistics_t
  {
    using list_t = std::vector<statistics_t>;
    size_t client_recv{};
    size_t client_drop{};
    size_t client_send{};
    size_t peer_recv{};
    size_t peer_drop{};
    size_t peer_send{};

    statistics_t &operator<< (statistics_t &that) noexcept
    {
      client_recv += that.client_recv;
      that.client_recv = 0;

      client_drop += that.client_drop;
      that.client_drop = 0;

      client_send += that.client_send;
      that.client_send = 0;

      peer_recv += that.peer_recv;
      that.peer_recv = 0;

      peer_drop += that.peer_drop;
      that.peer_drop = 0;

      peer_send += that.peer_send;
      that.peer_send = 0;

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
      client.start_receive_from(async.make_io());
      peer.start_receive_from(async.make_io());
    }
  }


  void on_client_recv (sal::net::async::io_t &&io,
    const socket_t::receive_from_t *event,
    statistics_t &statistics)
  {
    if (event->transferred == sizeof(session_map::key_type))
    {
      sessions.lock()->try_emplace(
        *reinterpret_cast<const session_map::key_type *>(io.data()),
        event->remote_endpoint
      );
      ++statistics.client_recv;
    }
    else
    {
      ++statistics.client_drop;
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
      io.resize(event->transferred);
      client.start_send_to(std::move(io), it->second);
      peer.start_receive_from(async.make_io());
      ++statistics.peer_recv;
    }
    else
    {
      peer.start_receive_from(std::move(io));
      ++statistics.peer_drop;
    }
  }
};


void service_t::run (size_t thread_index)
{
  auto worker = async.make_worker(poll_result_count);
  auto &statistics = thread_statistics[thread_index];

  for (;;)
  {
    if (auto io = worker.poll())
    {
      if (auto recv = socket_t::receive_from_result(io))
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
        if (io.socket_context<socket_t>() == &peer)
        {
          ++statistics.peer_send;
        }
        else
        {
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

  std::cout
    << "sessions: " << sessions_count
    << "\tclient: "
      << sum.client_recv
      << '/'
      << sum.client_send
      << '/'
      << sum.client_drop
    << "\tpeer: "
      << sum.peer_recv
      << '/'
      << sum.peer_send
      << '/'
      << sum.peer_drop
  ;

  total << sum;

  std::cout
    << "\ttotal: "
      << total.peer_recv
      << '/'
      << total.client_send
      << '/'
      << static_cast<int>(total.peer_recv - total.client_send)
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
    .add({"c", "completions"},
      requires_argument("INT", completions_count),
      help("completions queue size")
    )
    .add({"p", "poll"},
      requires_argument("INT", poll_result_count),
      help("max completions per poll")
    )
    .add({"r", "receives"},
      requires_argument("INT", receive_count),
      help("max outstanding receives")
    )
    .add({"s", "sends"},
      requires_argument("INT", send_count),
      help("max outstanding sends")
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

  std::cout << "completions: ";
  completions_count = std::stoul(
    options.back_or_default("completions", {arguments})
  );
  std::cout << completions_count << '\n';

  std::cout << "       poll: ";
  poll_result_count = std::stoul(
    options.back_or_default("poll", {arguments})
  );
  std::cout << poll_result_count << '\n';

  std::cout << "   receives: ";
  receive_count = std::stoul(
    options.back_or_default("receives", {arguments})
  );
  std::cout << receive_count << '\n';

  std::cout << "      sends: ";
  send_count = std::stoul(
    options.back_or_default("sends", {arguments})
  );
  std::cout << send_count << '\n';

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
