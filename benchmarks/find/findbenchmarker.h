#pragma once

#include "event_counter.h"
#include <atomic>
event_collector collector;

template <class function_type>
event_aggregate bench(const function_type &&function, size_t min_repeat = 10,
                      size_t min_time_ns = 400'000'000,
                      size_t max_repeat = 1000000) {
  size_t N = min_repeat;
  if (N == 0) {
    N = 1;
  }
  // We warm up first. We warmup for at least 0.4s (by default). This makes
  // sure that the processor is in a consistent state.
  event_aggregate warm_aggregate{};
  for (size_t i = 0; i < N; i++) {
    std::atomic_thread_fence(std::memory_order_acquire);
    collector.start();
    function();
    std::atomic_thread_fence(std::memory_order_release);
    event_count allocate_count = collector.end();
    warm_aggregate << allocate_count;
    if ((i + 1 == N) && (warm_aggregate.total_elapsed_ns() < min_time_ns) &&
        (N < max_repeat)) {
      N *= 10;
    }
  }
  // Actual measure, another 0.4s (by default), this time with a processor
  // warmed up.
  event_aggregate aggregate{};
  for (size_t i = 0; i < N; i++) {
    std::atomic_thread_fence(std::memory_order_acquire);
    collector.start();
    function();
    std::atomic_thread_fence(std::memory_order_release);
    event_count allocate_count = collector.end();
    aggregate << allocate_count;
    if ((i + 1 == N) && (aggregate.total_elapsed_ns() < min_time_ns) &&
        (N < max_repeat)) {
      N *= 10;
    }
  }
  return aggregate;
}
