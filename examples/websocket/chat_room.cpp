//
// examples/websocket/chat_room.cpp
//
// Chat-room style WebSocket server.
//
// This example demonstrates how to design a simple room-based protocol
// on top of the typed WebSocket messages.
//
// The server does not keep complex state about rooms; instead, it relies on
// the payload to carry room information, and broadcasts messages to everyone.
//
// Expected client messages:
//
//   1) Join room:
//
//      {
//        "type": "room.join",
//        "payload": {
//          "user": "Alice",
//          "room": "general"
//        }
//      }
//
//   2) Send message:
//
//      {
//        "type": "room.message",
//        "payload": {
//          "user": "Alice",
//          "room": "general",
//          "text": "Hello room!"
//        }
//      }
//
//   3) Typing indicator (optional):
//
//      {
//        "type": "room.typing",
//        "payload": {
//          "user": "Alice",
//          "room": "general"
//        }
//      }
//

#include <vix/config/Config.hpp>
#include <vix/experimental/ThreadPoolExecutor.hpp>
#include <vix/websocket.hpp>
#include <variant>

int main()
{
    using vix::websocket::Server;

    // ------------------------------------------------------------
    // 1) Load config
    // ------------------------------------------------------------
    vix::config::Config cfg{"config/config.json"};

    // ------------------------------------------------------------
    // 2) Thread pool for async WebSocket processing
    // ------------------------------------------------------------
    auto exec = vix::experimental::make_threadpool_executor(
        4, // min threads
        8, // max threads
        0  // default priority
    );

    // ------------------------------------------------------------
    // 3) Construct the WebSocket server
    // ------------------------------------------------------------
    Server ws(cfg, std::move(exec));

    // ------------------------------------------------------------
    // 4) On new connection
    // ------------------------------------------------------------
    ws.on_open(
        [&ws](auto &session)
        {
            (void)session;

            ws.broadcast_json(
                "room.system",
                {
                    "user",
                    "server",
                    "text",
                    "A new user connected to the chat room server 👋",
                });
        });

    // ------------------------------------------------------------
    // 5) Typed messages for room protocol
    // ------------------------------------------------------------
    ws.on_typed_message(
        [&ws](auto &session,
              const std::string &type,
              const vix::json::kvs &payload)
        {
            (void)session;

            // Small helper: extract a string value from kvs by key
            auto get_string = [](const vix::json::kvs &kv,
                                 const std::string &key) -> std::string
            {
                const auto &flat = kv.flat;

                // kvs is stored as [key, value, key, value, ...]
                for (std::size_t i = 0; i + 1 < flat.size(); i += 2)
                {
                    // Try to read the key token as string
                    if (auto keyStr = std::get_if<std::string>(&flat[i].v))
                    {
                        if (*keyStr == key)
                        {
                            // Try to read the value token as string
                            if (auto valStr = std::get_if<std::string>(&flat[i + 1].v))
                                return *valStr;
                        }
                    }
                }
                return {};
            };

            if (type == "room.join")
            {
                // payload: { "user": "Alice", "room": "general" }
                const std::string user = get_string(payload, "user");
                const std::string room = get_string(payload, "room");
                const std::string text = user + " joined room " + room;

                ws.broadcast_json("room.system",
                                  {"user", user,
                                   "room", room,
                                   "text", text});
            }
            else if (type == "room.message")
            {
                // payload: { "user": "Alice", "room": "general", "text": "..." }
                // Broadcast to all clients; each client can filter by "room".
                ws.broadcast_json("room.message", payload);
            }
            else if (type == "room.typing")
            {
                // payload: { "user": "Alice", "room": "general" }
                ws.broadcast_json("room.typing", payload);
            }
            else
            {
                ws.broadcast_json("room.unknown",
                                  {"type", type,
                                   "info", "Unknown room message type"});
            }
        });

    // ------------------------------------------------------------
    // 6) Run server (blocking)
    // ------------------------------------------------------------
    ws.listen_blocking();

    return 0;
}
