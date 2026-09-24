/**
 *
 * @file task_timeout.cpp
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
  vix::threadpool::ThreadPool pool(2);

  vix::threadpool::TaskOptions options;
  options.set_timeout(vix::threadpool::Timeout::milliseconds(50));

  auto future =
      pool.submit(
          []()
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 42;
          },
          options);

  try
  {
    const int value = future.get();
    std::cout << "Result: " << value << '\n';
  }
  catch (const std::exception &e)
  {
    std::cout << "Task failed: " << e.what() << '\n';
  }

  std::cout << "Future status: "
            << vix::threadpool::to_string(future.status())
            << '\n';

  std::cout << "Future result: "
            << vix::threadpool::to_string(future.result())
            << '\n';

  pool.shutdown();

  return 0;
}
