#include <bench/bench.hpp>
#include <sal/lockable.hpp>
#include <sal/net/async/completion_queue.hpp>
#include <sal/net/async/service.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/time.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <unordered_map>


namespace {

using namespace std::chrono;
using namespace std::chrono_literals;
using sal::program_options::option_set_t;
using sal::program_options::argument_map_t;

using protocol_t = sal::net::ip::udp_t;
using socket_t = protocol_t::socket_t;
using endpoint_t = socket_t::endpoint_t;
using service_t = sal::net::async::service_t;

auto address = sal::net::ip::make_address("0.0.0.0");
size_t thread_count = std::thread::hardware_concurrency();

size_t udp_header_size = address.is_v4() ? 28 : 48;
constexpr size_t receives_per_thread = 20;


struct io_stats_t
{
  static constexpr seconds print_interval = 2s;
  sal::time_t next_periodic_print_time_{};
  std::atomic<size_t> sessions{};

  struct atomic_io_t
  {
    std::atomic<size_t> bytes{}, packets{}, errors{};
  } sent{}, received{};

  struct io_t
  {
    size_t bytes{}, packets{}, errors{};

    io_t () = default;

    io_t (const atomic_io_t &io)
      : bytes(io.bytes)
      , packets(io.packets)
      , errors(io.errors)
    { }
  } last_sent{}, last_received{};


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

    stream << "sessions: " << sessions;

    stream << "  |  recv: ";
    print(stream, current_received, last_received);

    stream << "  |  send: ";
    print(stream, current_sent, last_sent);

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
      << " / "
      << (current.errors - last.errors) / print_interval.count() << " error(s)"
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


class relay_t
{
public:

  relay_t (io_stats_t &io_stats) noexcept
    : io_stats_(io_stats)
  {
    set_port_sharing_options(client_);
    client_.bind(alloc_endpoint_);
    client_.associate(service_);
    client_.context(&client_);

    peer_.associate(service_);
    peer_.context(&peer_);
  }


  void start ();


  void stop ()
  {
    for (auto &thread: threads_)
    {
      thread.join();
    }

    // start per thread number of receives
  }


private:

  io_stats_t &io_stats_;
  service_t service_{};

  const endpoint_t alloc_endpoint_{address, 3478};
  socket_t client_{address.is_v4() ? protocol_t::v4 : protocol_t::v6};

  const endpoint_t relayed_endpoint_{address, 3479};
  socket_t peer_{relayed_endpoint_};

  std::vector<std::thread> threads_{};

  using session_map = std::unordered_map<uint64_t, socket_t>;
  session_map sessions_data_{10'000};
  sal::lockable_t<session_map> sessions_{sessions_data_};

  static void handle_completions (relay_t &relay) noexcept;

  void on_client_receive (
    sal::net::async::io_ptr &&io,
    const socket_t::receive_from_t *receive_from
  );

  void on_peer_receive (
    sal::net::async::io_ptr &&io,
    const socket_t::receive_from_t *receive_from
  );

  void set_port_sharing_options (socket_t &socket)
  {
    #if __sal_os_macos
      socket.set_option(sal::net::reuse_port(true));
    #else
      socket.set_option(sal::net::reuse_address(true));
    #endif
  }
};


void relay_t::start ()
{
  while (threads_.size() < thread_count)
  {
    threads_.emplace_back(&relay_t::handle_completions, std::ref(*this));
  }

  // continuous number of receives on 3478
  auto client_receive_count = threads_.size() * receives_per_thread;
  while (client_receive_count--)
  {
    client_.start_receive_from(service_.make_io());
  }

  // initial number of receives on 3479
  // with each new session, add one more (on_client_receive)
  auto initial_peer_receive_count = receives_per_thread;
  while (initial_peer_receive_count--)
  {
    peer_.start_receive_from(service_.make_io());
  }
}


void relay_t::on_client_receive (sal::net::async::io_ptr &&io,
  const socket_t::receive_from_t *receive_from)
{
  if (receive_from->transferred == sizeof(session_map::key_type))
  {
    socket_t session{address.is_v4() ? protocol_t::v4 : protocol_t::v6};
    set_port_sharing_options(session);
    session.bind(alloc_endpoint_);
    session.connect(receive_from->remote_endpoint);
    session.associate(service_);

    sessions_.lock()->try_emplace(
      *reinterpret_cast<const session_map::key_type *>(io->data()),
      std::move(session)
    );
    ++io_stats_.sessions;

    // start new receive_from for each new session
    peer_.start_receive_from(service_.make_io());
  }

  // restart completed receive_from
  client_.start_receive_from(std::move(io));
}


void relay_t::on_peer_receive (sal::net::async::io_ptr &&io,
  const socket_t::receive_from_t *receive_from)
{
  if (receive_from->transferred >= sizeof(session_map::key_type))
  {
    auto it = sessions_.lock()->find(
      *reinterpret_cast<const session_map::key_type *>(io->data())
    );
    if (it != sessions_data_.end())
    {
      auto &session = it->second;
      io->resize(receive_from->transferred);
      session.start_send(std::move(io));

      // new peer start_receive_from will be triggered when
      // session.start_send() has completed
      return;
    }
  }

  // unknown data arrived at peer port, ignore and start new receive_from()
  peer_.start_receive_from(std::move(io));
}


void relay_t::handle_completions (relay_t &relay) noexcept
{
  auto &service = relay.service_;
  sal::net::async::completion_queue_t queue(service);
  std::error_code error;

  for (auto io = queue.try_get();  /**/;  io = queue.try_get())
  {
    if (io)
    {
      if (auto receive_from = io->get_if<socket_t::receive_from_t>(error))
      {
        auto &io_stats = relay.io_stats_.received;

        if (!error)
        {
          ++io_stats.packets;
          io_stats.bytes += receive_from->transferred + udp_header_size;
        }
        else
        {
          ++io_stats.errors;
          error.clear();
        }

        if (io->socket_context<socket_t>() == &relay.peer_)
        {
          relay.on_peer_receive(std::move(io), receive_from);
        }
        else
        {
          relay.on_client_receive(std::move(io), receive_from);
        }
      }
      else if (auto send = io->get_if<socket_t::send_t>(error))
      {
        auto &io_stats = relay.io_stats_.sent;
        if (!error)
        {
          ++io_stats.packets;
          io_stats.bytes += send->transferred + udp_header_size;
        }
        else
        {
          ++io_stats.errors;
          error.clear();
        }

        io->reset();
        relay.peer_.start_receive_from(std::move(io));
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
  constexpr auto align = 12;

  std::cout << std::setw(align) << "address: ";
  address = sal::net::ip::make_address(
    options.back_or_default("address", {arguments})
  );
  udp_header_size = address.is_v4() ? 28 : 48;
  std::cout << address << '\n';

  std::cout << std::setw(align) << "threads: ";
  thread_count = std::stoul(
    options.back_or_default("threads", {arguments})
  );
  sal_throw_if(thread_count < 1);
  std::cout << thread_count << '\n';
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
    .add({"t", "threads"},
      requires_argument("INT", thread_count),
      help("number of threads"
        " (default " + std::to_string(thread_count) + ')'
      )
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  setup_and_print_options(options, arguments);

  io_stats_t io_stats;
  relay_t relay(io_stats);

  relay.start();

  for (auto now = sal::now();  /**/;  now = sal::now())
  {
    io_stats.periodic_print(std::cout, now);
    std::this_thread::sleep_for(io_stats.print_interval);
  }

  relay.stop();

  return EXIT_SUCCESS;
}


} // namespace bench
