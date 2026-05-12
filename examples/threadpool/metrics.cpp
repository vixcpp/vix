/**
 *
 * @file metrics.cpp
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
#include <chrono>
#include <iostream>
#include <thread>

#include <vix/threadpool/all.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  for (int i = 0; i < 8; ++i)
  {
    pool.post(
        []()
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{20});
        });
  }

  pool.wait_idle();

  const vix::threadpool::ThreadPoolMetrics metrics = pool.metrics();
  const vix::threadpool::ThreadPoolStats stats = pool.stats();

  std::cout << "Worker count: " << metrics.worker_count << '\n';
  std::cout << "Pending tasks: " << metrics.pending_tasks << '\n';
  std::cout << "Active tasks: " << metrics.active_tasks << '\n';
  std::cout << "Completed tasks: " << metrics.completed_tasks << '\n';
  std::cout << "Failed tasks: " << metrics.failed_tasks << '\n';
  std::cout << "Rejected tasks: " << metrics.rejected_tasks << '\n';
  std::cout << "Idle: " << (metrics.idle() ? "yes" : "no") << '\n';

  std::cout << "Accepted tasks: " << stats.accepted_tasks << '\n';
  std::cout << "Finished tasks: " << stats.finished_tasks() << '\n';
  std::cout << "Error tasks: " << stats.error_tasks() << '\n';

  pool.shutdown();

  return 0;
}
