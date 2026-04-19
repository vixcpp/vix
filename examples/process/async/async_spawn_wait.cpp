/**
 *
 *  @file async_spawn_wait.cpp
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
#include <vix/process/async/SpawnAsync.hpp>
#include <vix/process/async/WaitAsync.hpp>

using namespace vix;

async::core::task<void> run(async::core::io_context &ctx)
{
  process::Command cmd("sleep");
  cmd.arg("1");

  std::cout << "spawning process...\n";

  auto child = co_await process::async::spawn(ctx, std::move(cmd));

  std::cout << "pid = " << child.id() << '\n';

  int exit_code = co_await process::async::wait(ctx, child);

  std::cout << "process exited with code = " << exit_code << '\n';

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
