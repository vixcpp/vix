/**
 *
 * @file parallel_for.cpp
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
#include <cstddef>
#include <iostream>
#include <mutex>
#include <vector>

#include <vix/threadpool/all.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(20, 0);

  vix::threadpool::parallel_for(
      pool,
      std::size_t{0},
      values.size(),
      [&values](std::size_t index)
      {
        values[index] = static_cast<int>(index * index);
      });

  pool.wait_idle();

  std::cout << "parallel_for result:";

  for (const int value : values)
  {
    std::cout << ' ' << value;
  }

  std::cout << '\n';

  pool.shutdown();

  return 0;
}
