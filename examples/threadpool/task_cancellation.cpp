/**
 *
 * @file task_cancellation.cpp
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

  auto handle =
      pool.handle(
          []()
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 7;
          });

  handle.cancel();

  try
  {
    const int value = handle.get();
    std::cout << "Result: " << value << '\n';
  }
  catch (const std::exception &e)
  {
    std::cout << "Task failed: " << e.what() << '\n';
  }

  std::cout << "Cancelled: "
            << (handle.cancelled() ? "yes" : "no")
            << '\n';

  std::cout << "Task status: "
            << vix::threadpool::to_string(handle.status())
            << '\n';

  pool.shutdown();

  return 0;
}
