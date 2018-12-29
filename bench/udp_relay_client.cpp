#include <bench/bench.hpp>
#include <sal/crypto/random.hpp>
#include <sal/net/async/completion_queue.hpp>
#include <sal/net/async/service.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/span.hpp>
#include <sal/time.hpp>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <thread>


namespace {

using namespace std::chrono;
using namespace std::chrono_literals;
using sal::program_options::option_set_t;
using sal::program_options::argument_map_t;

using protocol_t = sal::net::ip::udp_t;
using socket_t = protocol_t::socket_t;
using endpoint_t = socket_t::endpoint_t;
using service_t = sal::net::async::service_t;

size_t udp_header_size = 28;

auto address = sal::net::ip::make_address("127.0.0.1");
size_t thread_count = 1;
size_t session_count = 1;
size_t session_bitrate_kbps = 32;
size_t packet_size_bytes = 200;


struct io_stats_t
{
  static constexpr seconds print_interval = 2s;
  sal::time_t next_periodic_print_time_{};

  struct atomic_io_t
  {
    std::atomic<size_t> packets{}, bytes{};
  } received{};

  struct io_t
  {
    size_t packets{}, bytes{};

    io_t () = default;

    io_t (const atomic_io_t &io_stats)
      : packets(io_stats.packets)
      , bytes(io_stats.bytes)
    {}
  } sent{}, last_sent{}, last_received{};


  void periodic_print (std::ostream &stream, const sal::time_t &now)
  {
    if (now >= next_periodic_print_time_)
    {
      print(stream);
      next_periodic_print_time_ = now + print_interval;
    }
  }


  void print (std::ostream &stream)
  {
    io_t current_received = received, current_sent = sent;

    stream << "send: ";
    print(stream, current_sent, last_sent);
    stream << "  |  recv: ";
    print(stream, current_received, last_received);
    stream << '\n';

    last_sent = current_sent;
    last_received = current_received;
  }


  void print (std::ostream &stream, const io_t &current, const io_t &last)
  {
    stream
      << (current.packets - last.packets) / print_interval.count() << "pps"
      << " / "
      << bits_per_sec((current.bytes - last.bytes) / print_interval.count())
    ;
  }


  std::string bits_per_sec (size_t Bps)
  {
    static constexpr std::array<const char *, 4> units
    {
      "bps", "Kbps", "Mbps", "Gbps"
    };
    auto unit = units.begin();

    auto bps = 8 * Bps;
    while (bps > 1000 && (unit + 1) != units.end())
    {
      bps /= 1000;
      unit++;
    }

    return std::to_string(bps) + *unit;
  }
};


class session_t
{
public:

  using list_t = std::vector<session_t>;


  session_t (size_t size, service_t &service)
    : service_(service)
    , size_(size)
  { }


  void connect (const endpoint_t &alloc_endpoint, const endpoint_t &relayed_endpoint)
  {
    client_.associate(service_);
    client_.context(this);
    client_.connect(alloc_endpoint);
    client_.send(sal::span(&id_, sizeof(id_)));
    client_.start_receive(service_.make_io(&client_));

    peer_.associate(service_);
    peer_.connect(relayed_endpoint);
  }


  void send (io_stats_t &stats)
  {
    auto io = service_.make_io(&peer_);
    io->resize(size_);
    io->skip_completion_notification(true);
    *reinterpret_cast<uint64_t *>(io->data()) = id_;
    peer_.start_send(std::move(io));
    stats.sent.bytes += size_ + udp_header_size;
    ++stats.sent.packets;
  }


  static list_t allocate_many (size_t count, size_t size, service_t &service);

  static void handle_completions (service_t &service, io_stats_t &io_stats)
    noexcept;


private:

  uint64_t id_ = gen_id();
  socket_t client_{protocol_t::v4}, peer_{protocol_t::v4};
  service_t &service_;
  size_t size_;

  uint64_t gen_id () noexcept
  {
    uint64_t id;
    sal::crypto::random(&id, &id + 1);
    return id;
  }
};


session_t::list_t session_t::allocate_many (
  size_t count,
  size_t size,
  service_t &service)
{
  list_t result;
  result.reserve(count);
  while (result.size() < count)
  {
    result.emplace_back(size, service);
  }
  return result;
}


void session_t::handle_completions (service_t &service, io_stats_t &io_stats)
  noexcept
{
  sal::net::async::completion_queue_t queue(service);
  for (auto io = queue.try_get();  /**/;  io = queue.try_get())
  {
    if (io)
    {
      if (auto receive = io->get_if<socket_t::receive_t>())
      {
        ++io_stats.received.packets;
        io_stats.received.bytes += receive->transferred + udp_header_size;
        auto &session = *sal_check_ptr(io->socket_context<session_t>());
        session.client_.start_receive(std::move(io));
      }
    }
    else
    {
      queue.wait();
    }
  }
}


void setup_and_print_options (
  const option_set_t &options,
  const argument_map_t &arguments)
{
  constexpr auto align = 18;

  std::cout << std::setw(align) << "address: ";
  address = sal::net::ip::make_address(
    options.back_or_default("address", {arguments})
  );
  udp_header_size = address.is_v4() ? 28 : 48;
  std::cout << address << '\n';

  std::cout << std::setw(align) << "thread count: ";
  thread_count = std::stoul(
    options.back_or_default("thread_count", {arguments})
  );
  sal_throw_if(thread_count < 1);
  std::cout << thread_count << '\n';

  std::cout << std::setw(align) << "session count: ";
  session_count = std::stoul(
    options.back_or_default("session_count", {arguments})
  );
  sal_throw_if(session_count < 1);
  std::cout << session_count << '\n';

  std::cout << std::setw(align) << "session bitrate: ";
  session_bitrate_kbps = std::stoul(
    options.back_or_default("session_bitrate", {arguments})
  );
  std::cout << session_bitrate_kbps << "kbps\n";

  std::cout << std::setw(align) << "packet size: ";
  packet_size_bytes = std::stoul(
    options.back_or_default("packet_size", {arguments})
  );
  std::cout << packet_size_bytes << "B\n";
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
      help("UDP relay server address"
        " (default " + address.to_string() + ')'
      )
    )
    .add({"t", "thread_count"},
      requires_argument("INT", thread_count),
      help("number of receiver threads"
        " (default " + std::to_string(thread_count) + ')'
      )
    )
    .add({"c", "session_count"},
      requires_argument("INT", session_count),
      help("number of sessions"
        " (default " + std::to_string(session_count) + ')'
      )
    )
    .add({"b", "session_bitrate"},
      requires_argument("INT", session_bitrate_kbps),
      help("session bitrate"
        " (default " + std::to_string(session_bitrate_kbps) + "kbps)"
      )
    )
    .add({"s", "packet_size"},
      requires_argument("INT", packet_size_bytes),
      help("packet size in bytes"
        " (default " + std::to_string(packet_size_bytes) + "B)"
      )
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  setup_and_print_options(options, arguments);

  service_t service;
  endpoint_t alloc_endpoint{address, 3478}, relayed_endpoint{address, 3479};
  io_stats_t io_stats;

  auto sessions = session_t::allocate_many(
    session_count,
    packet_size_bytes,
    service
  );

  for (auto &session: sessions)
  {
    session.connect(alloc_endpoint, relayed_endpoint);
  }

  std::vector<std::thread> threads;
  while (threads.size() < thread_count)
  {
    threads.emplace_back(
      &session_t::handle_completions,
      std::ref(service),
      std::ref(io_stats)
    );
  }

  size_t bytes_per_msec = session_count * 128 * session_bitrate_kbps / 1000;
  auto it = sessions.begin();

  for (auto now = sal::now(), started = now;  /**/;  now = sal::now())
  {
    io_stats.periodic_print(std::cout, now);

    auto passed = duration_cast<milliseconds>(now - started);
    auto bytes_to_send = passed.count() * bytes_per_msec;

    if (bytes_to_send <= io_stats.sent.bytes)
    {
      std::this_thread::sleep_for(4ms);
      continue;
    }

    while (bytes_to_send > io_stats.sent.bytes)
    {
      it->send(io_stats);
      if (++it == sessions.end())
      {
        it = sessions.begin();
      }
    }
  }

  for (auto &thread: threads)
  {
    thread.join();
  }

  return EXIT_SUCCESS;
}


} // namespace bench
