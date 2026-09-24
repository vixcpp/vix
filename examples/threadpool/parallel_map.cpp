/**
 *
 * @file parallel_map.cpp
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
#include <string>
#include <vector>

#include <vix/threadpool/all.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> numbers{1, 2, 3, 4, 5, 6, 7, 8};

  std::vector<int> squares =
      vix::threadpool::parallel_map(
          pool,
          numbers,
          [](int value)
          {
            return value * value;
          });

  std::cout << "Squares:";

  for (const int value : squares)
  {
    std::cout << ' ' << value;
  }

  std::cout << '\n';

  pool.shutdown();

  return 0;
}
