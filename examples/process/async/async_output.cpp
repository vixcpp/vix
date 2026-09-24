/**
 *
 *  @file async_output.cpp
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
#include <utility>

#include <vix/async/async.hpp>

#include <vix/process/Command.hpp>
#include <vix/process/async/OutputAsync.hpp>

using namespace vix;

async::core::task<void> run(async::core::io_context &ctx)
{
  process::Command cmd("echo");
  cmd.arg("hello from async output");

  auto result = co_await process::async::output(ctx, std::move(cmd));

  std::cout << "exit code = " << result.exit_code << '\n';
  std::cout << "stdout = " << result.stdout_text << '\n';
  std::cout << "stderr = " << result.stderr_text << '\n';

  ctx.stop();
  co_return;
}

int main()
{
  async::core::io_context ctx;
  async::core::spawn_detached(ctx, run(ctx));
  ctx.run();
  return 0;
}
