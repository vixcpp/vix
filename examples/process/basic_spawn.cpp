/**
 *
 *  @file basic_spawn.cpp
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
  command.arg("hello from vix::process");

  auto result = vix::process::spawn(command);

  if (!result)
  {
    std::cerr << "spawn failed: " << result.error().message() << '\n';
    return 1;
  }

  std::cout << "spawned process id = " << result.value().id() << '\n';
  return 0;
}
