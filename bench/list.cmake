list(APPEND sal_bench
  # main
  bench/bench.hpp
  bench/main.cpp
)

list(APPEND sal_bench_modules
  # modules
  bench/logger.cpp
  bench/queue.cpp
  bench/spinlock.cpp
  bench/str.cpp
)
