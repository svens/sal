list(APPEND sal_bench_sources
  # main
  bench/bench.hpp
  bench/main.cpp
)

list(APPEND sal_bench_modules
  # modules
  bench/intrusive_mpsc_queue.cpp
  bench/logger.cpp
  bench/memory_writer.cpp
  bench/queue.cpp
  bench/spinlock.cpp
  bench/udp_echo_client.cpp
  bench/udp_echo_server.cpp
)
