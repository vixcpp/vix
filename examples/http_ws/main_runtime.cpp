//
// examples/http_ws/main_runtime.cpp
//
// High-level example using vix::serve_http_and_ws().
//
// This is the most friendly API for Node.js / Python developers:
//   - One call to serve_http_and_ws(config, port, lambda)
//   - Inside the lambda you configure HTTP (app) and WebSocket (ws).
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
            // HTTP routes (Express / FastAPI style)
            // --------------------------------------------------------

            app.get("/", [](auto &, auto &res)
                    { res.json({"framework", "Vix.cpp",
                                "mode", "runtime",
                                "message", "HTTP + WebSocket via serve_http_and_ws() 🚀"}); });

            app.get("/hello/{name}", [](auto &, auto &res, auto &params)
                    { res.json({"greeting", "Hello " + params["name"] + " 👋",
                                "powered_by", "Vix.cpp"}); });

            // --------------------------------------------------------
            // WebSocket handlers
            // --------------------------------------------------------

            // Fired when a client opens a WebSocket connection
            ws.on_open([&ws](auto &session)
                       {
                (void)session;

                ws.broadcast_json("chat.system", {
                    "user", "server",
                    "text", "Welcome to the runtime example! 👋"
                }); });

            // Fired on every typed message: { "type": "...", "payload": {...} }
            ws.on_typed_message(
                [&ws](auto &session,
                      const std::string &type,
                      const vix::json::kvs &payload)
                {
                    (void)session;

                    if (type == "chat.message")
                    {
                        // Echo / broadcast chat messages to all clients
                        ws.broadcast_json("chat.message", payload);
                    }
                });
        });

    return 0;
}
