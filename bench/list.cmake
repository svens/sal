list(APPEND sal_bench
  # main
  bench/bench.hpp
  bench/main.cpp
)

list(APPEND sal_bench_modules
  # modules
  bench/array_string.cpp
  bench/intrusive_queue.cpp
  bench/logger.cpp
  bench/queue.cpp
  bench/spinlock.cpp
)
