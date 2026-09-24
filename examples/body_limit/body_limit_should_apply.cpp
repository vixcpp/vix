/**
 *
 *  @file  body_limit_should_apply.cpp — CORS + conditional body limit via should_apply()
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
// Run:
//   vix run vix/examples/body_limit_should_apply.cpp
//
// Front (serve from another origin):
//   python3 -m http.server 5173 --bind 127.0.0.1
//   open http://localhost:5173
//
// Curl tests:
//   curl -i http://localhost:8080/health
//   curl -i -X POST http://localhost:8080/upload --data "hello"
//   curl -i -X POST http://localhost:8080/upload --data "0123456789abcdefX"   # 17 bytes => 413
//
// CORS preflight tests:
//   curl -i -X OPTIONS http://localhost:8080/api/echo \
//     -H "Origin: http://localhost:5173" \
//     -H "Access-Control-Request-Method: POST" \
//     -H "Access-Control-Request-Headers: content-type"
//
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

// ------------------------------------------------------------
// Install CORS + OPTIONS routes (so browser preflight works)
// ------------------------------------------------------------
static void install_cors(App &app)
{
  app.use("/", middleware::app::cors_ip_demo({"http://localhost:5173",
                                              "http://127.0.0.1:5173",
                                              "http://0.0.0.0:5173"}));

  auto options_noop = [](Request &, Response &res)
  {
    res.status(204).send();
  };

  app.options("/api/ping", options_noop);
  app.options("/api/echo", options_noop);
  app.options("/api/strict", options_noop);
  app.options("/upload", options_noop);
}

// ------------------------------------------------------------
// Install body limit (write methods only) — via alias preset
// ------------------------------------------------------------
static void install_body_limit(App &app)
{
  // Applies only to POST/PUT/PATCH (alias)
  app.use("/", middleware::app::body_limit_write_dev(16));
}

// ------------------------------------------------------------
// Routes
// ------------------------------------------------------------
static void install_routes(App &app)
{
  app.get("/health", [](Request &, Response &res)
          { res.json({"ok", true}); });

  app.get("/api/ping", [](Request &, Response &res)
          {
        res.header("X-Request-Id", "req_ping_1");
        res.json({"ok", true, "msg", "pong"}); });

  app.post("/api/echo", [](Request &req, Response &res)
           {
        res.header("X-Request-Id", "req_echo_1");
        res.json({
            "ok", true,
            "path", req.path(),
            "bytes", static_cast<long long>(req.body().size()),
            "body", req.body()
        }); });

  app.post("/api/strict", [](Request &req, Response &res)
           { res.json({"ok", true, "bytes", static_cast<long long>(req.body().size())}); });

  app.post("/upload", [](Request &req, Response &res)
           { res.json({"ok", true, "bytes", static_cast<long long>(req.body().size())}); });
}

static void run_app()
{
  App app;

  install_cors(app);
  install_body_limit(app);
  install_routes(app);

  app.run(8080);
}

int main()
{
  run_app();
  return 0;
}
