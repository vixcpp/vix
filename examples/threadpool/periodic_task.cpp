/**
 *
 * @file periodic_task.cpp
 * @author Gaspard Kirira
 *
 * Copyright 2025, Gaspard Kirira.
 * All rights reserved.
 * https://github.com/vixcpp/vix
 *
 * Use of this source code is governed by a MIT license
 * that can be found in the License file.
 *
 * Vix.cpp
 *
 */
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include <vix/threadpool/all.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> ticks{0};

  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{100};
  config.run_immediately = true;

  vix::threadpool::PeriodicTask task(
      pool,
      [&ticks]()
      {
        const int current = ticks.fetch_add(1, std::memory_order_relaxed) + 1;
        std::cout << "Tick: " << current << '\n';
      },
      config);

  if (!task.start())
  {
    std::cout << "Failed to start periodic task\n";
    pool.shutdown();
    return 1;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds{450});

  task.stop();
  task.join();

  pool.wait_idle();

  std::cout << "Submitted ticks: " << task.submitted_ticks() << '\n';
  std::cout << "Failed posts: " << task.failed_posts() << '\n';
  std::cout << "Observed ticks: " << ticks.load() << '\n';

  pool.shutdown();

  return 0;
}
