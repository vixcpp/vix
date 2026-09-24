/**
 *
 *  @file async_pipeline.cpp
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
#include <vix/process/pipeline/PipelineAsync.hpp>

using namespace vix;

async::core::task<void> run(async::core::io_context &ctx)
{
  process::Command producer("echo");
  producer.arg("hello from pipeline");

  process::Command consumer("cat");

  auto children =
      co_await process::pipeline::async::spawn(ctx, std::move(producer), std::move(consumer));

  auto result =
      co_await process::pipeline::async::wait(ctx, children);

  std::cout << "first exit code = " << result.first_exit_code << '\n';
  std::cout << "second exit code = " << result.second_exit_code << '\n';

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
