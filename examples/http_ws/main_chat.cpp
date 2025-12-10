//
// examples/http_ws/main_chat.cpp
//
// Chat-style example on top of HTTP + WebSocket.
//
// Demonstrates:
//   - HTTP JSON endpoints
//   - WebSocket typed messages for chat:
//       * "chat.join"    → user joined
//       * "chat.message" → broadcast chat message
//       * "chat.typing"  → typing indicator
//

#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
    vix::serve_http_and_ws(
        "config/config.json", 8080,
        [](auto &app, auto &ws)
        {
            // --------------------------------------------------------
            // HTTP: basic API for client bootstrapping
            // --------------------------------------------------------

            app.get("/", [](auto &, auto &res)
                    { res.json({"name", "Vix Chat Example",
                                "description", "HTTP + WebSocket powered chat server",
                                "framework", "Vix.cpp"}); });

            app.get("/health", [](auto &, auto &res)
                    { res.json({"status", "ok",
                                "service", "chat",
                                "version", "1.0.0"}); });

            // --------------------------------------------------------
            // WebSocket: chat protocol
            //
            // Client messages are expected as:
            // {
            //   "type": "chat.join" | "chat.message" | "chat.typing",
            //   "payload": {
            //       "user": "Alice",
            //       "text": "...",        // for chat.message
            //       "room": "general"    // optional
            //   }
            // }
            // --------------------------------------------------------

            ws.on_open([&ws](auto &session)
                       {
                (void)session;

                // Notify everybody that a new connection is here
                ws.broadcast_json("chat.system", {
                    "user", "server",
                    "text", "A new user connected to the chat 👋"
                }); });

            ws.on_typed_message(
                [&ws](auto &session,
                      const std::string &type,
                      const vix::json::kvs &payload)
                {
                    (void)session;

                    if (type == "chat.join")
                    {
                        // Example: { "user": "Alice", "room": "general" }
                        ws.broadcast_json("chat.system", {"user", payload.at("user"),
                                                          "text", payload.at("user") + " joined the chat 🚀"});
                    }
                    else if (type == "chat.message")
                    {
                        // Example: { "user": "Alice", "text": "Hello!" }
                        ws.broadcast_json("chat.message", payload);
                    }
                    else if (type == "chat.typing")
                    {
                        // Example: { "user": "Alice" }
                        ws.broadcast_json("chat.typing", payload);
                    }
                    else
                    {
                        // Unknown type → optional debug
                        ws.broadcast_json("chat.unknown", {"info", "Unknown message type",
                                                           "type", type});
                    }
                });
        });

    return 0;
}
