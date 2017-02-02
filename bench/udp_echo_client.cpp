#include <bench/bench.hpp>
#include <sal/net/internet.hpp>
#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>


using namespace std::chrono;
using namespace std::chrono_literals;


namespace {

using protocol_t = sal::net::ip::udp_t;
using socket_t = protocol_t::socket_t;

// configuration
socket_t::endpoint_t server_endpoint(
  sal::net::ip::make_address_v4("127.0.0.1"), 8192
);
size_t packet_size = 1024, interval_ms = 100;


using sys_clock_t = steady_clock;
using sys_time_t = sys_clock_t::time_point;


struct packet_info_t
{
  sys_time_t send_time;
};


void received (const packet_info_t &packet)
{
  static std::vector<sys_time_t::rep> history;
  static size_t received = 0, history_index = 0;
  static auto rtt = 0us;

  // single packet stats
  auto now = sys_clock_t::now();
  auto this_rtt = duration_cast<microseconds>(now - packet.send_time);
  rtt += this_rtt;
  ++received;

  // keep last 100 packet RTT history
  if (history.size() == 100)
  {
    history[history_index++] = this_rtt.count();
    if (history_index == 100)
    {
      history_index = 0;
    }
  }
  else
  {
    history.emplace_back(this_rtt.count());
  }

  // print stats every second
  static auto next_report = now + 1s;
  if (next_report > now)
  {
    return;
  }
  next_report = now + 1s;

  auto dev = std::make_pair(0.0, 0.0);
  for (auto &h: history)
  {
    dev.first += h * h;
    dev.second += h;
  }
  dev.first /= history.size();
  dev.second /= history.size();
  auto jitter = std::sqrt(dev.first - dev.second * dev.second);
  rtt /= received;

  std::cout
    << "received=" << received
    << "; rtt=" << duration_cast<milliseconds>(rtt).count() << "ms"
    << "; jitter=" << std::setprecision(2) << std::fixed << jitter/1000 << "ms"
    << '\n'
  ;

  rtt = 0us;
  received = 0;
}


} // namespace


namespace bench {


option_set_t options ()
{
  using namespace sal::program_options;

  option_set_t desc;
  desc
    .add({"a", "address"},
      requires_argument("ADDRESS", "127.0.0.1"),
      help("UDP echo server IPv4 address")
    )
    .add({"i", "interval"},
      requires_argument("INT", interval_ms),
      help("interval of packet generation (in milliseconds)")
    )
    .add({"p", "port"},
      requires_argument("INT", server_endpoint.port()),
      help("listening port")
    )
    .add({"s", "size"},
      requires_argument("INT", packet_size),
      help("packet size")
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  server_endpoint.address(
    sal::net::ip::make_address_v4(
      options.back_or_default("address", { arguments })
    )
  );
  server_endpoint.port(
    static_cast<sal::net::ip::port_t>(
      std::stoul(options.back_or_default("port", { arguments }))
    )
  );

  packet_size = std::stoul(options.back_or_default("size", { arguments }));
  if (packet_size > sal::net::io_buf_t::max_size())
  {
    packet_size = sal::net::io_buf_t::max_size();
    std::cout << "enforcing maximum packet size " << packet_size << "B\n";
  }
  else if (packet_size < sizeof(packet_info_t))
  {
    packet_size = sizeof(packet_info_t);
    std::cout << "enforcing minimum packet size " << packet_size << "B\n";
  }

  milliseconds interval(
    std::stoul(options.back_or_default("interval", { arguments }))
  );

  sal::net::io_service_t io_svc;
  socket_t socket(protocol_t::v4());
  io_svc.associate(socket);

  // reader thread
  auto reader = std::thread([&io_svc, &socket]
  {
    auto io_ctx = io_svc.make_context(64);
    while (auto io_buf = io_ctx.get())
    {
      if (auto recv = socket_t::async_receive_from_result(io_buf))
      {
        auto &packet = *reinterpret_cast<packet_info_t *>(io_buf->data());
        received(packet);
        io_buf->reset();
        socket.async_receive_from(std::move(io_buf));
      }
    }
  });

  // generate packets
  auto io_ctx = io_svc.make_context();
  bool receive_started = false;
  while (true)
  {
    auto io_buf = io_ctx.make_buf();
    io_buf->resize(packet_size);

    auto &packet = *reinterpret_cast<packet_info_t *>(io_buf->data());
    packet.send_time = sys_clock_t::now();
    socket.async_send_to(std::move(io_buf), server_endpoint);

    if (!receive_started)
    {
      // send some data to server sender socket (to create map in NAT)
      socket_t::endpoint_t endpoint(server_endpoint.address(), server_endpoint.port() + 1);
      socket.async_send_to(io_ctx.make_buf(), endpoint);

      // now that we have bound to ephemeral port, start receives as well
      for (auto i = 0;  i < 64;  ++i)
      {
        socket.async_receive_from(io_ctx.make_buf());
      }
      receive_started = true;
    }

    if (interval.count())
    {
      std::this_thread::sleep_for(interval);
    }

    io_ctx.reclaim();
  }

  return EXIT_SUCCESS;
}


} // namespace bench
