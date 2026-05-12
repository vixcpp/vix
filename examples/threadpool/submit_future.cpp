/**
 *
 * @file submit_future.cpp
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

#include <vix/threadpool/all.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto numberFuture =
      pool.submit(
          []()
          {
            return 42;
          });

  auto textFuture =
      pool.submit(
          []()
          {
            return std::string{"Hello from Vix threadpool"};
          });

  const int number = numberFuture.get();
  const std::string text = textFuture.get();

  std::cout << "Number result: " << number << '\n';
  std::cout << "Text result: " << text << '\n';

  pool.shutdown();

  return 0;
}
