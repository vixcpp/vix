/**
 * @file simple_server.cpp
 * @brief Minimal WebSocket server example for Vix.cpp
 *
 * This file provides a compact, beginner-friendly demonstration of how to
 * start a WebSocket server using the Vix.cpp runtime. It serves as the
 * simplest reference implementation, showing only the essential components:
 *
 *  • Loading configuration (port, timeouts, etc.)
 *  • Creating a ThreadPoolExecutor for async execution
 *  • Starting a WebSocket server instance
 *  • Reacting to connection events (on_open)
 *  • Handling typed JSON messages (on_typed_message)
 *  • Broadcasting messages to all connected clients
 *
 * Key Concepts Illustrated
 * -------------------------
 * 1. Server Initialization:
 *      The WebSocket server automatically binds to the port defined in
 *      config/config.json and manages all asynchronous I/O.
 *
 * 2. ThreadPool Integration:
 *      The example uses Vix’s ThreadPoolExecutor to handle concurrency,
 *      enabling scalable real-time workloads even in minimal setups.
 *
 * 3. Global Broadcast:
 *      Messages received with type "chat.message" are broadcast to all
 *      connected clients without room routing or persistence.
 *
 * 4. System Notifications:
 *      On connection open, the server emits a "chat.system" JSON message
 *      welcoming new clients to the Softadastra network.
 *
 * Intended Usage
 * --------------
 * This minimal example is ideal for:
 *
 *  • Developers learning the basics of Vix.cpp WebSockets
 *  • Quick prototypes and internal tools
 *  • Testing client applications
 *  • Teaching how typed WebSocket protocols work in C++
 *
 * How to Run
 * ----------
 * 1. Ensure your config file exists:
 *        config/config.json
 *
 * 2. Build the project:
 *        cmake -S . -B build && cmake --build build -j
 *
 * 3. Run the server:
 *        ./build/examples/simple/simple_server
 *
 * 4. Connect using a WebSocket client:
 *        websocat ws://127.0.0.1:9090/
 *
 * This example is intentionally minimal—use the advanced examples for
 * persistence, metrics, room routing, history replay, and auto-reconnect.
 */

#include <vix/config/Config.hpp>
#include <vix/experimental/ThreadPoolExecutor.hpp>
#include <vix/websocket.hpp>

int main()
{
    using vix::websocket::Server;

    // Load configuration from config/config.json
    vix::config::Config cfg{"config/config.json"};

    // Thread pool for async work
    auto exec = vix::experimental::make_threadpool_executor(
        4, // min threads
        8, // max threads
        0  // default priority
    );

    Server ws(cfg, std::move(exec));

    // On new connection: broadcast a welcome system message
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
                    "welcome to Softadastra Chat 👋",
                });
        });

    // On typed message: echo chat messages to everyone
    ws.on_typed_message(
        [&ws](auto &session,
              const std::string &type,
              const vix::json::kvs &payload)
        {
            (void)session;

            if (type == "chat.message")
            {
                ws.broadcast_json("chat.message", payload);
            }
        });

    ws.listen_blocking();
}