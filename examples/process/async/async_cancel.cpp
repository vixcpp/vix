/**
 *
 *  @file async_cancel.cpp
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

#include <chrono>
#include <iostream>
#include <system_error>
#include <utility>

#include <vix/async/async.hpp>

#include <vix/process/Command.hpp>
#include <vix/process/async/SpawnAsync.hpp>
#include <vix/process/async/WaitAsync.hpp>

using namespace vix;

async::core::task<void> run(async::core::io_context &ctx)
{
  async::core::cancel_source cancel;

  process::Command cmd("sleep");
  cmd.arg("5");

  auto child = co_await process::async::spawn(ctx, std::move(cmd));

  std::cout << "started process pid = " << child.id() << '\n';

  ctx.timers().after(
      std::chrono::seconds(1),
      [&cancel]()
      {
        std::cout << "requesting cancel...\n";
        cancel.request_cancel();
      });

  try
  {
    int code = co_await process::async::wait(ctx, child, cancel.token());
    std::cout << "exit code = " << code << '\n';
  }
  catch (const std::system_error &e)
  {
    std::cout << "operation cancelled: " << e.what() << '\n';
  }

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
