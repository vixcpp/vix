/**
 *
 *  @file context.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */

#include <vix/log/Log.hpp>

int main()
{
  vix::log::LogContext ctx;
  ctx.request_id = "req-abc-123";
  ctx.module = "auth";
  ctx.fields["user_id"] = "42";
  ctx.fields["ip"] = "127.0.0.1";

  vix::log::set_context(ctx);
  vix::log::set_level(vix::log::LogLevel::Info);

  vix::log::info("user authenticated successfully");
  vix::log::logf(
      vix::log::LogLevel::Info,
      "structured auth log",
      "status", 200,
      "method", "POST",
      "path", "/login");

  vix::log::clear_context();
  return 0;
}
