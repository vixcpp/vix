/**
 *
 *  @file examples/websocket/02_http_ws_basic/main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  HTTP + WebSocket basic example.
 *  Goal:
 *    - start a Vix HTTP app and WebSocket server together
 *    - expose simple HTTP routes
 *    - accept typed WebSocket messages
 *    - keep the file small but structured
 *
 */

#include <memory>
#include <string>
#include <utility>

#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

namespace
{
  /**
   * @brief Small facade that owns the shared runtime objects used by the example.
   *
   * This keeps main() very small and makes the bootstrap flow explicit.
   */
  struct BasicRuntime
  {
    vix::config::Config config{".env"};
    std::shared_ptr<vix::executor::RuntimeExecutor> executor{
        std::make_shared<vix::executor::RuntimeExecutor>(1u)};
    vix::App app{executor};
    vix::websocket::Server ws{config, executor};
  };

  /**
   * @brief Register the HTTP routes for the example.
   */
  void register_http_routes(vix::App &app)
  {
    app.get(
        "/",
        [](vix::Request &, vix::Response &res)
        {
          res.json({
              {"name", "Vix HTTP + WebSocket Basic"},
              {"message", "HTTP and WebSocket are running together"},
              {"framework", "Vix.cpp"},
          });
        });

    app.get(
        "/health",
        [](vix::Request &, vix::Response &res)
        {
          res.json({
              {"status", "ok"},
              {"service", "http-ws-basic"},
          });
        });

    app.get(
        "/hello/{name}",
        [](vix::Request &req, vix::Response &res)
        {
          const std::string name = req.param("name", "world");

          res.json({
              {"message", "Hello " + name},
              {"source", "http"},
          });
        });
  }

  /**
   * @brief Register the WebSocket lifecycle handlers.
   */
  void register_ws_lifecycle(vix::websocket::Server &ws)
  {
    ws.on_open(
        [&ws](vix::websocket::Session &session)
        {
          (void)session;

          ws.broadcast_json(
              "system.connected",
              {
                  "message",
                  "A WebSocket client connected",
              });
        });

    ws.on_close(
        [](vix::websocket::Session &session)
        {
          (void)session;
        });

    ws.on_error(
        [](vix::websocket::Session &session, const std::string &error)
        {
          (void)session;
          (void)error;
        });
  }

  /**
   * @brief Register the typed WebSocket protocol used by this example.
   *
   * Supported messages:
   *   - app.ping      -> emits app.pong
   *   - chat.message  -> rebroadcasts chat.message
   *   - any other type -> emits app.unknown
   */
  void register_ws_protocol(vix::websocket::Server &ws)
  {
    ws.on_typed_message(
        [&ws](
            vix::websocket::Session &session,
            const std::string &type,
            const vix::json::kvs &payload)
        {
          (void)session;

          if (type == "app.ping")
          {
            ws.broadcast_json(
                "app.pong",
                {
                    "status",
                    "ok",
                    "transport",
                    "websocket",
                });

            return;
          }

          if (type == "chat.message")
          {
            ws.broadcast_json("chat.message", payload);
            return;
          }

          ws.broadcast_json(
              "app.unknown",
              {
                  "type",
                  type,
                  "message",
                  "Unknown event type",
              });
        });
  }

  /**
   * @brief Configure all HTTP and WebSocket parts of the runtime.
   */
  void configure(BasicRuntime &runtime)
  {
    register_http_routes(runtime.app);
    register_ws_lifecycle(runtime.ws);
    register_ws_protocol(runtime.ws);
  }

  /**
   * @brief Run the composed HTTP + WebSocket runtime.
   */
  void run(BasicRuntime &runtime)
  {
    const int http_port = runtime.config.getServerPort();
    vix::run_http_and_ws(runtime.app, runtime.ws, runtime.executor, http_port);
  }
} // namespace

int main()
{
  BasicRuntime runtime;
  configure(runtime);
  run(runtime);
  return 0;
}
