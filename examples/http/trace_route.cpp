/**
 *
 *  @file examples/http/trace_route.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */

// ============================================================================
// GET /trace -> {"rid": "<uuid>", "ok": true}
// ============================================================================

#include <vix.hpp>
#include <vix/utils/Logger.hpp>
#include <vix/utils/UUID.hpp>
#include <string>

using namespace vix;
using namespace vix::utils;

int main()
{
  auto &log = Logger::getInstance();
  log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
  log.setLevel(Logger::Level::INFO);
  log.setAsync(true);

  App app;

  // GET /trace
  app.get("/trace", [](Request &req, Response &res)
          {
        Logger::Context ctx;
        ctx.request_id = utils::uuid4();
        ctx.module     = "trace_handler";
        Logger::getInstance().setContext(ctx);

        std::string path(req.target().data(), req.target().size());
        std::string method(req.method().data(), req.method().size());

        Logger::getInstance().logf(
            Logger::Level::INFO,
            "Incoming request",
            "path", path.c_str(),
            "method", method.c_str(),
            "rid", ctx.request_id.c_str());

        res.json({
            "rid", ctx.request_id,
            "ok",  true
        }); });

  app.run(8080);
}
