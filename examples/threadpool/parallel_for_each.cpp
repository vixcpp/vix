/**
 *
 * @file parallel_for_each.cpp
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
#include <string>
#include <vector>

#include <vix/threadpool/all.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<std::string> names{
      "vix",
      "threadpool",
      "parallel",
      "runtime"};

  std::mutex mutex;

  vix::threadpool::parallel_for_each(
      pool,
      names,
      [&mutex](const std::string &name)
      {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "Processing: " << name << '\n';
      });

  pool.wait_idle();
  pool.shutdown();

  return 0;
}
