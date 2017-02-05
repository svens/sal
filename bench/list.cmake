list(APPEND sal_bench
  # main
  bench/bench.hpp
  bench/main.cpp
)

list(APPEND sal_bench_modules
  # modules
  bench/intrusive_queue.cpp
  bench/logger.cpp
  bench/memory_writer.cpp
  bench/queue.cpp
  bench/spinlock.cpp
)

if(WIN32)
  list(APPEND sal_bench_modules
    bench/udp_echo_client.cpp
    bench/udp_echo_server.cpp
  )
endif()
