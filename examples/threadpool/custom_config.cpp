/**
 *
 * @file custom_config.cpp
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
  vix::threadpool::ThreadPoolConfig config;

  config.thread_count = 2;
  config.max_thread_count = 2;
  config.max_queue_size = 16;
  config.default_priority = vix::threadpool::TaskPriority::normal;
  config.allow_dynamic_growth = false;
  config.drain_on_shutdown = true;
  config.swallow_task_exceptions = true;
  config.default_timeout = std::chrono::milliseconds{0};

  vix::threadpool::ThreadPool pool(config);

  auto first =
      pool.submit(
          []()
          {
            return 10;
          });

  auto second =
      pool.submit(
          []()
          {
            return 32;
          });

  const int result = first.get() + second.get();

  std::cout << "Thread count: " << pool.thread_count() << '\n';
  std::cout << "Result: " << result << '\n';

  const vix::threadpool::ThreadPoolMetrics metrics = pool.metrics();

  std::cout << "Completed tasks: " << metrics.completed_tasks << '\n';
  std::cout << "Rejected tasks: " << metrics.rejected_tasks << '\n';

  pool.shutdown();

  return 0;
}
