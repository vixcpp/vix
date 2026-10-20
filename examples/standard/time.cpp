/**
 * @file time.cpp
 * @brief Demonstrates Vix durations, clocks, and deadlines.
 *
 * Build (from the Vix repository root):
 * g++ -std=c++20 -I standard/include examples/standard/time.cpp -o time-example
 */

#include <vix/standard.hpp>

#include <iostream>
#include <thread>

int main()
{
  using vix::Duration;

  // A Duration is a length of time, not a point on a clock.
  const auto budget = Duration::milliseconds(100);
  const auto extension = Duration::milliseconds(50);
  const auto total = budget + extension;

  std::cout << "Time budget: "
            << total.nanosecond_count() / 1'000'000
            << " ms\n";

  // WallClock gives calendar/system time, suitable for timestamps.
  // It can change when the system clock is adjusted.
  const auto timestamp = vix::WallClock::now();
  std::cout << "Unix timestamp (ns): "
            << timestamp.unix_nanosecond_count() << '\n';

  // MonotonicClock is suitable for measuring elapsed time.
  const auto start = vix::MonotonicClock::now();
  const auto deadline = vix::Deadline::after(start, budget);

  // Simulate a short operation; sleeping may last longer than requested.
  std::this_thread::sleep_for(Duration::milliseconds(20).to_chrono());

  const auto now = vix::MonotonicClock::now();
  const auto elapsed = now - start;

  std::cout << "Elapsed: "
            << elapsed.nanosecond_count() / 1'000'000
            << " ms\n";

  // A deadline is a monotonic boundary; checking it does not stop work.
  if (deadline.reached(now))
  {
    std::cout << "Deadline reached\n";
  }
  else
  {
    const auto remaining = deadline.remaining(now);
    std::cout << "Remaining: "
              << remaining.nanosecond_count() / 1'000'000
              << " ms\n";
  }

  return 0;
}
