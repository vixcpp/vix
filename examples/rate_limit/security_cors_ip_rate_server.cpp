/**
 *
 *  @file security_cors_ip_rate_server.cpp — CORS + IP Filter + Rate Limit (Vix.cpp)
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
===============================================================================
CURL TESTS
===============================================================================
# 0) Run server
vix run security_cors_ip_rate_server.cpp

# 1) Preflight allowed origin (should be 204 + Access-Control-Allow-Origin)
curl -i -X OPTIONS http://localhost:8080/api/echo \
  -H "Origin: http://localhost:5173" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type"

# 2) Preflight blocked origin (should be 403 from CORS middleware)
curl -i -X OPTIONS http://localhost:8080/api/echo \
  -H "Origin: https://evil.com" \
  -H "Access-Control-Request-Method: POST"

# 3) IP denied (deny=1.2.3.4) => 403 ip_denied
curl -i http://localhost:8080/api/ping \
  -H "Origin: http://localhost:5173" \
  -H "X-Forwarded-For: 1.2.3.4"

# 4) Allowed IP (not denied) => 200 OK
curl -i http://localhost:8080/api/ping \
  -H "Origin: http://localhost:5173" \
  -H "X-Forwarded-For: 9.9.9.9"

# 5) Rate limit demo (capacity=5, refill=0): 6th request => 429
for i in $(seq 1 6); do
  echo "---- $i"
  curl -i http://localhost:8080/api/ping \
    -H "Origin: http://localhost:5173" \
    -H "X-Forwarded-For: 9.9.9.9"
done
*/

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static void register_options_routes(App &app)
{
  // Without explicit OPTIONS routes, Vix core may auto-return 204
  // BEFORE middleware runs => preflight has no CORS headers.

  app.options("/api/ping", [](Request &, Response &res)
              {
        res.header("X-OPTIONS-HIT", "ping");
        res.status(204).send(); });

  app.options("/api/echo", [](Request &, Response &res)
              {
        res.header("X-OPTIONS-HIT", "echo");
        res.status(204).send(); });
}

int main()
{
  App app;

  // Apply all on /api prefix (ORDER MATTERS)
  app.use("/api", middleware::app::cors_ip_demo());
  app.use("/api", middleware::app::ip_filter_dev("x-vix-ip", {"1.2.3.4"}));
  app.use("/api", middleware::app::rate_limit_custom_dev(
                      5.0,       // capacity (burst)
                      0.0,       // refill_per_sec (demo: easy to trigger)
                      "x-vix-ip" // key header (must match IP filter header)
                      ));

  // Public
  app.get("/", [](Request &, Response &res)
          { res.send("public route"); });

  // Critical for browser preflight headers
  register_options_routes(app);

  app.get("/api/ping", [](Request &req, Response &res)
          {
        res.header("X-Request-Id", "req_ping_1");
        res.json({
            "ok", true,
            "msg", "pong",
            "ip", req.header("x-vix-ip")
        }); });

  app.post("/api/echo", [](Request &req, Response &res)
           {
        res.header("X-Request-Id", "req_echo_1");
        res.json({
            "ok", true,
            "msg", "echo",
            "content_type", req.header("content-type")
        }); });

  app.run(8080);
}
