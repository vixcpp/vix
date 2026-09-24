/**
 *
 *  @file capture_output.cpp
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
  command.arg("hello from output");
  command.stdout_mode(vix::process::PipeMode::Pipe);
  command.stderr_mode(vix::process::PipeMode::Pipe);

  auto result = vix::process::output(command);
  if (!result)
  {
    std::cerr << "output failed: " << result.error().message() << '\n';
    return 1;
  }

  std::cout << "exit code = " << result.value().exit_code << '\n';
  std::cout << "stdout = " << result.value().stdout_text << '\n';
  std::cout << "stderr = " << result.value().stderr_text << '\n';
  return 0;
}
