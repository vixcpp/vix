/**
 *
 *  @file json_app_simple.cpp — JSON parser (App) simple example (Vix.cpp)
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
// ----------------------------------------------------------------------------
// Run:
//   vix run json_app_simple.cpp
//
// Test:
//   # OK
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: application/json; charset=utf-8" \
//     --data '{"x":1}'
//
//   # Invalid JSON -> 400
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: application/json" \
//     --data '{"x":}'
//
//   # Invalid content-type -> 415
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: text/plain" \
//     --data '{"x":1}'
//
//   # Empty body (allowed here) -> 200 + {}
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: application/json" \
//     --data ""
//
//   # Payload too large -> 413 (max_bytes demo)
//   BIG="$(python3 -c 'print("{\"x\":\"" + "a"*300 + "\"}")')"
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: application/json" \
//     --data "$BIG"
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // 1-liner like Node/FastAPI
  app.use("/json", middleware::app::json_dev(
                       /*max_bytes=*/256,
                       /*allow_empty=*/true,
                       /*require_content_type=*/true));

  app.get("/", [](Request &, Response &res)
          { res.send("POST /json with application/json"); });

  app.post("/json", [](Request &req, Response &res)
           {
                 auto &jb = req.state<middleware::parsers::JsonBody>();

                 // keep it simple: just echo the parsed JSON
                 res.json({
                     "ok", true,
                     "raw", jb.value.dump()
                 }); });

  app.run(8080);
  return 0;
}
