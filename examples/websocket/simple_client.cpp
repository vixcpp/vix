//
// examples/websocket/simple_server.cpp
//
// Minimal WebSocket server using Vix.cpp.
//
// This example demonstrates:
//   - how to bootstrap the WebSocket Server
//   - how to handle connection open events
//   - how to handle typed messages and broadcast JSON
//
// Expected message format (typed protocol):
//   {
//     "type": "chat.message",
//     "payload": { "user": "Alice", "text": "Hello!" }
//   }
//

#include <vix/config/Config.hpp>
#include <vix/experimental/ThreadPoolExecutor.hpp>
#include <vix/websocket.hpp>

int main()
{
    using vix::websocket::Server;

    // ------------------------------------------------------------
    // 1) Load configuration
    // ------------------------------------------------------------
    //
    // The Config loader will try to find "config/config.json"
    // relative to the project root or the current working directory.
    //
    vix::config::Config cfg{"config/config.json"};

    // ------------------------------------------------------------
    // 2) Create a thread pool executor for the WebSocket server
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
                "chat.system",
                {
                    "user",
                    "server",
                    "text",
                    "Welcome to the simple WebSocket server 👋",
                });
        });

    // ------------------------------------------------------------
    // 5) On typed message
    // ------------------------------------------------------------
    ws.on_typed_message(
        [&ws](auto &session,
              const std::string &type,
              const vix::json::kvs &payload)
        {
            (void)session;

            if (type == "chat.message")
            {
                // echo / broadcast chat messages
                ws.broadcast_json("chat.message", payload);
            }
            else
            {
                // optional: broadcast unknown message types for debugging
                ws.broadcast_json("chat.unknown",
                                  {"type", type,
                                   "info", "Unknown message type"});
            }
        });

    // ------------------------------------------------------------
    // 6) Start the WebSocket server (blocking)
    // ------------------------------------------------------------
    ws.listen_blocking();

    return 0;
}
