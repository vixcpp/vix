/**
 *
 * @file parallel_reduce.cpp
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
#include <numeric>
#include <vector>

#include <vix/threadpool/all.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> numbers(100);

  std::iota(numbers.begin(), numbers.end(), 1);

  const int sum =
      vix::threadpool::parallel_reduce(
          pool,
          numbers,
          0,
          [](int current, int value)
          {
            return current + value;
          });

  std::cout << "Sum: " << sum << '\n';

  pool.shutdown();

  return 0;
}
