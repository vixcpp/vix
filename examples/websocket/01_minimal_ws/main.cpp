/**
 *
 *  @file examples/websocket/01_minimal_ws/main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  Minimal WebSocket server example.
 *  Goal:
 *    - start a native Vix WebSocket server
 *    - accept connections
 *    - echo raw text messages
 *    - echo typed JSON messages: { "type": "...", "payload": {...} }
 *
 */

#include <memory>
#include <string>

#include <vix.hpp>
#include <vix/websocket.hpp>

namespace
{
  std::shared_ptr<vix::executor::RuntimeExecutor> create_executor()
  {
    return std::make_shared<vix::executor::RuntimeExecutor>(1u);
  }

  void register_open_handler(vix::websocket::Server &ws)
  {
    ws.on_open(
        [&ws](vix::websocket::Session &session)
        {
          (void)session;

          ws.broadcast_json(
              "system.connected",
              {
                  "message",
                  "A client connected",
              });
        });
  }

  void register_close_handler(vix::websocket::Server &ws)
  {
    ws.on_close(
        [](vix::websocket::Session &session)
        {
          (void)session;
        });
  }

  void register_error_handler(vix::websocket::Server &ws)
  {
    ws.on_error(
        [](vix::websocket::Session &session, const std::string &error)
        {
          (void)session;
          (void)error;
        });
  }

  void register_raw_message_handler(vix::websocket::Server &ws)
  {
    ws.on_message(
        [&ws](vix::websocket::Session &session, const std::string &payload)
        {
          (void)session;

          ws.broadcast_json(
              "echo.raw",
              {
                  "text",
                  payload,
              });
        });
  }

  void register_typed_message_handler(vix::websocket::Server &ws)
  {
    ws.on_typed_message(
        [&ws](
            vix::websocket::Session &session,
            const std::string &type,
            const vix::json::kvs &payload)
        {
          (void)session;

          ws.broadcast_json(type, payload);
        });
  }

  void configure_server(vix::websocket::Server &ws)
  {
    register_open_handler(ws);
    register_close_handler(ws);
    register_error_handler(ws);
    register_raw_message_handler(ws);
    register_typed_message_handler(ws);
  }
} // namespace

int main()
{
  vix::config::Config config{".env"};
  auto executor = create_executor();

  vix::websocket::Server ws{config, executor};

  configure_server(ws);

  ws.listen_blocking();

  return 0;
}
