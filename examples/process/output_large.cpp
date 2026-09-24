/**
 *
 *  @file output_large.cpp
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

#include <algorithm>
#include <iostream>
#include <string>

#include <vix/process/Command.hpp>
#include <vix/process/Output.hpp>

namespace
{
  std::string preview(const std::string &text, std::size_t max_len = 200)
  {
    if (text.size() <= max_len)
    {
      return text;
    }

    return text.substr(0, max_len) + "...";
  }
}

int main()
{
  using namespace vix;

#ifndef _WIN32
  process::Command cmd("sh");
  cmd.arg("-c");
  cmd.arg(
      "i=1; "
      "while [ \"$i\" -le 2000 ]; do "
      "  echo \"stdout line $i\"; "
      "  echo \"stderr line $i\" 1>&2; "
      "  i=$((i+1)); "
      "done");
#else
  process::Command cmd("cmd");
  cmd.arg("/C");
  cmd.arg(
      "for /L %i in (1,1,2000) do @("
      "echo stdout line %i & "
      "echo stderr line %i 1>&2)");
#endif

  auto result = process::output(cmd);
  if (!result)
  {
    std::cerr << "output failed: "
              << result.error().message() << '\n';
    return 1;
  }

  const auto &out = result.value();

  std::cout << "exit code      = " << out.exit_code << '\n';
  std::cout << "stdout size    = " << out.stdout_text.size() << " bytes\n";
  std::cout << "stderr size    = " << out.stderr_text.size() << " bytes\n";

  std::cout << "\nstdout preview:\n"
            << preview(out.stdout_text) << '\n';

  std::cout << "\nstderr preview:\n"
            << preview(out.stderr_text) << '\n';

  const bool has_stdout_marker =
      out.stdout_text.find("stdout line 1") != std::string::npos;
  const bool has_stderr_marker =
      out.stderr_text.find("stderr line 1") != std::string::npos;

  std::cout << "\nstdout captured = "
            << (has_stdout_marker ? "true" : "false") << '\n';
  std::cout << "stderr captured = "
            << (has_stderr_marker ? "true" : "false") << '\n';

  return out.success() ? 0 : 1;
}
