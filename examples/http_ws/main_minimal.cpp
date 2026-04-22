/**
 *
 *  @file examples/http_ws/main_minimal.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp
 *
 */

#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

int main()
{
  // Minimal HTTP + WebSocket runtime using .env config
  vix::serve_http_and_ws(
      [](auto &app, auto &ws)
      {
        // HTTP: simplest possible route
        app.get(
            "/",
            [](auto &, auto &res)
            {
              res.text("OK");
            });

        // WebSocket: echo everything (no protocol)
        ws.on_typed_message(
            [&ws](
                auto &session,
                const std::string &type,
                const vix::json::kvs &payload)
            {
              (void)session;

              ws.broadcast_json(type, payload);
            });
      });

  return 0;
}
