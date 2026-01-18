/**
 *
 *  @file examples/http_ws/main_basic.cpp
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

#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
  // Construct the HTTP App + WebSocket Server together
  // The config file path can be omitted; if omitted,
  // Vix will automatically look for config/config.json.
  //
  auto bundle = vix::make_http_and_ws("config/config.json");
  auto &[app, ws] = bundle;

  // GET /
  app.get("/", [](Request &, Response &res)
          { res.json({"framework", "Vix.cpp",
                      "message", "HTTP + WebSocket example (basic) 🚀"}); });

  // GET /hello/{name}
  app.get("/hello/{name}", [](Request &req, Response &res)
          { res.json({"greeting", "Hello " + req.param("name") + " 👋",
                      "powered_by", "Vix.cpp"}); });

  // Register WebSocket event handlers
  ws.on_open([&ws](auto &session)
             {
        (void)session;

        ws.broadcast_json("chat.system", {
            "user", "server",
            "text", "Welcome to Vix WebSocket! 👋"
        }); });

  // When a typed message is received:
  ws.on_typed_message(
      [&ws](auto &session,
            const std::string &type,
            const vix::json::kvs &payload)
      {
        (void)session;

        // Basic chat echo example
        if (type == "chat.message") {
            ws.broadcast_json("chat.message", payload);
        } });

  // 4) Start HTTP + WebSocket together
  // This function:
  //   - runs the WebSocket server in a background thread
  //   - installs a shutdown callback on the HTTP server
  //   - blocks on app.run(port)
  //
  vix::run_http_and_ws(app, ws, 8080);

  return 0;
}
