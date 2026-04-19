/**
 *
 *  @file check_status.cpp
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
  vix::process::Command command("echo");
  command.arg("status example");

  auto spawned = vix::process::spawn(command);
  if (!spawned)
  {
    std::cerr << "spawn failed: " << spawned.error().message() << '\n';
    return 1;
  }

  auto running = vix::process::status(spawned.value());
  if (!running)
  {
    std::cerr << "status failed: " << running.error().message() << '\n';
    return 1;
  }

  std::cout << "process running = " << (running.value() ? "true" : "false") << '\n';
  return 0;
}
