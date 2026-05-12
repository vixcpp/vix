/**
 *
 * @file task_group.cpp
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
  vix::threadpool::TaskGroup group;

  group.add_task(1);
  group.add_task(2);
  group.add_task(3);

  std::thread first(
      [&group]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        group.finish_task(
            vix::threadpool::TaskStatus::completed,
            vix::threadpool::TaskResult::success);
      });

  std::thread second(
      [&group]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(40));

        group.finish_task(
            vix::threadpool::TaskStatus::completed,
            vix::threadpool::TaskResult::success);
      });

  std::thread third(
      [&group]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));

        group.finish_task(
            vix::threadpool::TaskStatus::cancelled,
            vix::threadpool::TaskResult::cancelled);
      });

  group.close();
  group.wait();

  first.join();
  second.join();
  third.join();

  std::cout << "Total tasks: " << group.total_tasks() << '\n';
  std::cout << "Completed: " << group.completed_tasks() << '\n';
  std::cout << "Cancelled: " << group.cancelled_tasks() << '\n';
  std::cout << "Has error: " << (group.has_error() ? "yes" : "no") << '\n';

  return 0;
}
