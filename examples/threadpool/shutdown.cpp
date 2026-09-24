/**
 *
 * @file shutdown.cpp
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

  for (int i = 0; i < 4; ++i)
  {
    pool.post(
        [i]()
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{50});
          std::cout << "Task finished: " << i << '\n';
        });
  }

  pool.wait_idle();

  std::cout << "Pool idle before shutdown: "
            << (pool.idle() ? "yes" : "no")
            << '\n';

  pool.shutdown();

  std::cout << "Pool running after shutdown: "
            << (pool.running() ? "yes" : "no")
            << '\n';

  const bool accepted =
      pool.post(
          []()
          {
            std::cout << "This should not run after shutdown\n";
          });

  std::cout << "Post after shutdown accepted: "
            << (accepted ? "yes" : "no")
            << '\n';

  return 0;
}
