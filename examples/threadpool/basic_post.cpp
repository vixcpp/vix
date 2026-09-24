/**
 *
 * @file basic_post.cpp
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
#include <iostream>

#include <vix/threadpool/all.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::atomic<int> counter{0};

  for (int i = 0; i < 10; ++i)
  {
    pool.post(
        [&counter]()
        {
          counter.fetch_add(1, std::memory_order_relaxed);
        });
  }

  pool.wait_idle();

  std::cout << "Executed tasks: " << counter.load() << '\n';

  pool.shutdown();

  return 0;
}
