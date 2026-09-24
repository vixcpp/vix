/**
 *
 *  @file pipeline_output.cpp
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
#include <string>

#include <vix/process/Command.hpp>
#include <vix/process/Output.hpp>

int main()
{
  using namespace vix;

#ifndef _WIN32
  process::Command cmd("sh");
  cmd.arg("-c");
  cmd.arg("printf 'orange\\napple\\nbanana\\n' | sort");
#else
  process::Command cmd("cmd");
  cmd.arg("/C");
  cmd.arg("(echo orange& echo apple& echo banana) | sort");
#endif

  auto result = process::output(std::move(cmd));
  if (!result)
  {
    std::cerr << "pipeline output failed: "
              << result.error().message() << '\n';
    return 1;
  }

  const auto &out = result.value();

  std::cout << "exit code = " << out.exit_code << '\n';
  std::cout << "stdout:\n"
            << out.stdout_text;
  std::cout << "stderr:\n"
            << out.stderr_text;

  return out.success() ? 0 : 1;
}
