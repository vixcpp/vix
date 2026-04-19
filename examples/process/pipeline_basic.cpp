/**
 *
 *  @file pipeline_basic.cpp
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
#include <vix/process/pipeline/Pipeline.hpp>

int main()
{
  using namespace vix;

  process::Command producer("echo");
  producer.arg("hello from pipeline");

  process::Command consumer("cat");

  auto spawned = process::pipeline::spawn(producer, consumer);
  if (!spawned)
  {
    std::cerr << "pipeline spawn failed: "
              << spawned.error().message() << '\n';
    return 1;
  }

  const auto &children = spawned.value();

  std::cout << "pipeline started\n";
  std::cout << "first pid  = " << children.first.id() << '\n';
  std::cout << "second pid = " << children.second.id() << '\n';

  auto waited = process::pipeline::wait(children);
  if (!waited)
  {
    std::cerr << "pipeline wait failed: "
              << waited.error().message() << '\n';
    return 1;
  }

  const auto &result = waited.value();

  std::cout << "first exit code  = " << result.first_exit_code << '\n';
  std::cout << "second exit code = " << result.second_exit_code << '\n';
  std::cout << "pipeline success = "
            << (result.success() ? "true" : "false") << '\n';

  return result.success() ? 0 : 1;
}
