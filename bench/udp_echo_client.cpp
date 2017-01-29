#include <bench/bench.hpp>
#include <sal/net/internet.hpp>
#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <thread>
#include <iostream>


namespace {

using protocol_t = sal::net::ip::udp_t;
using socket_t = protocol_t::socket_t;

// configuration
socket_t::endpoint_t server_endpoint(
  sal::net::ip::make_address_v4("127.0.0.1"), 8192
);
size_t packet_size = 1024, interval_ms = 100;


struct packet_info_t
{
  size_t index;
};


#if 0
void print_stats (size_t active_threads, size_t packets, size_t size_bytes)
{
  std::ostringstream oss;
  oss
    << "threads: " << active_threads
    << "; packets: " << packets
  ;

  size_t bps = 8 * size_bytes;
  const char units[] = " kMG";
  const char *unit = &units[0];

  while (size_bytes >= 1024)
  {
    size_bytes /= 1024;
    bps /= 1000;
    unit++;
  }

  oss << "; " << *unit << "bps=" << bps
    << "; " << *unit << "Bps=" << size_bytes
    << '\n';
  ;

  static std::string output;
  if (output != oss.str())
  {
    output = oss.str();
    std::cout << output;
  }
}
#endif


} // namespace


namespace bench {


using namespace std::chrono_literals;


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

  std::chrono::milliseconds interval(
    std::stoul(options.back_or_default("interval", { arguments }))
  );

  sal::net::io_service_t io_svc;
  socket_t socket(protocol_t::v4());
  io_svc.associate(socket);

  size_t send_index = 0;

  // reader thread
  auto reader = std::thread([&io_svc, &socket]
  {
    auto io_ctx = io_svc.make_context(64);
    while (auto io_buf = io_ctx.get())
    {
      if (auto recv = socket_t::async_receive_from_result(io_buf))
      {
        auto &packet = *reinterpret_cast<packet_info_t *>(io_buf->data());
        (void)packet;
        io_buf->reset();
        socket.async_receive_from(std::move(io_buf));
      }
    }
  });

  // generate packets
  auto io_ctx = io_svc.make_context();
  while (true)
  {
    auto io_buf = io_ctx.make_buf();
    io_buf->resize(packet_size);

    auto &packet = *reinterpret_cast<packet_info_t *>(io_buf->data());
    packet.index = send_index++;
    socket.async_send_to(std::move(io_buf), server_endpoint);

    if (send_index == 1)
    {
      // now that we have bound to ephemeral port, start receives as well
      for (auto i = 0;  i < 64;  ++i)
      {
        socket.async_receive_from(io_ctx.make_buf());
      }
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
