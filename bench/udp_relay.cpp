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
  std::vector<size_t> thread_statistics;

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


  void on_client_data (sal::net::async::io_t &&io,
    const socket_t::receive_from_t *event)
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


  void on_peer_data (sal::net::async::io_t &&io, size_t &forward_count)
  {
    auto it = sessions.lock()->find(
      *reinterpret_cast<const session_map::key_type *>(io.data())
    );
    if (it != sessions_data.end())
    {
      client.start_send_to(std::move(io), it->second);
      peer.start_receive_from(async.make_io());
      ++forward_count;
    }
    else
    {
      peer.start_receive_from(std::move(io));
    }
  }
};


void service_t::run (size_t thread_index)
{
  auto worker = async.make_worker(poll_result_count);
  auto &forward_count = thread_statistics[thread_index];

  for (;;)
  {
    if (auto io = worker.poll())
    {
      if (auto recv = socket_t::receive_from_result(io))
      {
        if (io.socket_context<socket_t>() == &peer)
        {
          on_peer_data(std::move(io), forward_count);
        }
        else
        {
          on_client_data(std::move(io), recv);
        }
      }
    }
  }
}


void service_t::print_statistics ()
{
  size_t sessions_count = sessions.lock()->size(), packets_count = 0;
  for (auto &count: thread_statistics)
  {
    packets_count += count;
    count = 0;
  }

  std::cout
    << "sessions: " << std::setw(10) << std::left << sessions_count
    << "packets: " << std::setw(10) << std::left << packets_count
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
