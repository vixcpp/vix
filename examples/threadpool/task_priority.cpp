/**
 *
 * @file task_priority.cpp
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
#include <iostream>
#include <mutex>
#include <vector>

#include <vix/threadpool/all.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  std::mutex mutex;
  std::vector<int> order;

  vix::threadpool::TaskOptions lowPriority;
  lowPriority.set_priority(vix::threadpool::TaskPriority::low);

  vix::threadpool::TaskOptions highPriority;
  highPriority.set_priority(vix::threadpool::TaskPriority::high);

  pool.post(
      [&mutex, &order]()
      {
        std::lock_guard<std::mutex> lock(mutex);
        order.push_back(1);
      },
      lowPriority);

  pool.post(
      [&mutex, &order]()
      {
        std::lock_guard<std::mutex> lock(mutex);
        order.push_back(2);
      },
      highPriority);

  pool.post(
      [&mutex, &order]()
      {
        std::lock_guard<std::mutex> lock(mutex);
        order.push_back(3);
      },
      highPriority);

  pool.wait_idle();

  std::cout << "Execution order:";

  for (const int value : order)
  {
    std::cout << ' ' << value;
  }

  std::cout << '\n';

  pool.shutdown();

  return 0;
}
