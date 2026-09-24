/**
 *
 *  @file terminate_process.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */

#include <iostream>

#include <vix/process/Process.hpp>

int main()
{
  vix::process::Command command("sleep");
  command.arg("5");

  auto spawned = vix::process::spawn(command);
  if (!spawned)
  {
    std::cerr << "spawn failed: " << spawned.error().message() << '\n';
    return 1;
  }

  auto err = vix::process::terminate(spawned.value());
  if (err)
  {
    std::cerr << "terminate failed: " << err.message() << '\n';
    return 1;
  }

  std::cout << "process termination requested: " << spawned.value().id() << '\n';
  return 0;
}
