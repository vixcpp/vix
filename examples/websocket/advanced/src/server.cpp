/**
 * @file server.cpp
 * @brief Advanced WebSocket server example for Vix.cpp
 *
 * This example demonstrates a fully–featured, production-style WebSocket
 * server using the Vix.cpp runtime. It showcases how to combine:
 *
 *  • Asynchronous WebSocket server (Beast + Asio)
 *  • ThreadPoolExecutor integration (high-performance scheduling)
 *  • Room-based messaging (join, leave, broadcast)
 *  • Typed JSON protocol ("type" + "payload")
 *  • Persistent message storage using SQLite (WAL enabled)
 *  • Automatic replay of chat history on join
 *  • Prometheus-compatible metrics server (/metrics endpoint)
 *  • Structured system events for room lifecycle (join/leave)
 *
 * Key Concepts Illustrated
 * -------------------------
 * 1. WebSocketMetrics:
 *      A lightweight metrics collector exposed via an HTTP endpoint.
 *      Metrics include connections, messages in/out, and error counts.
 *
 * 2. run_metrics_server():
 *      A minimal Beast-based HTTP server exposing Prometheus text format.
 *      Runs independently from the WebSocket server in a detached thread.
 *
 * 3. SqliteMessageStore:
 *      Provides durable message persistence with replay support.
 *      Uses WAL mode for crash safety and high write throughput.
 *
 * 4. Typed Message Handling:
 *      The WebSocket server routes messages based on their "type" field:
 *          • chat.join    – joins a room + replays history + broadcasts notice
 *          • chat.leave   – leaves a room + broadcasts notice
 *          • chat.message – persists and broadcasts user messages
 *          • other types  – fallback handler for custom events
 *
 * 5. Room Broadcasts:
 *      Room membership is automatically managed by Vix.cpp.
 *      The example shows how to broadcast to a single room or globally.
 *
 * 6. Offline-First Design:
 *      Messages are appended to SQLite before being broadcast,
 *      enabling reliable replay, reconnect recovery, and audit logging.
 *
 * Intended Usage
 * --------------
 * This example is designed for developers building:
 *
 *  • Real-time chat systems
 *  • Collaboration tools
 *  • Event-driven dashboards
 *  • IoT or telemetry streaming
 *  • Any system requiring durable WebSocket channels
 *
 * It demonstrates recommended architectural patterns for Vix.cpp
 * WebSocket applications, including:
 *
 *  • async I/O separation
 *  • metrics visibility
 *  • persistence layering
 *  • structured JSON protocols
 *  • minimalistic yet production-ready design
 *
 * How to Run
 * ----------
 *  1. Ensure dependencies are built: Vix.cpp, nlohmann/json, SQLite3.
 *  2. Create a config file: config/config.json (with websocket.port, etc.)
 *  3. Compile the example:
 *         cmake -S . -B build && cmake --build build -j
 *  4. Run:
 *         ./build/examples/advanced/server
 *  5. Connect using a WebSocket client:
 *         websocat ws://127.0.0.1:9090/
 *  6. Scrape metrics:
 *         curl http://127.0.0.1:9100/metrics
 *
 * This file is part of the Vix.cpp WebSocket module examples and is meant
 * to serve as a reference for building robust real-time systems in C++20.
 */
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <cstdint>

#include <nlohmann/json.hpp>

#include <vix.hpp> // HTTP runtime (App, vhttp::ResponseWrapper, etc.)

#include <vix/websocket.hpp>
#include <vix/websocket/Metrics.hpp>
#include <vix/websocket/SqliteMessageStore.hpp>
#include <vix/websocket/protocol.hpp>
#include <vix/websocket/LongPolling.hpp>
#include <vix/websocket/LongPollingBridge.hpp>

#include <boost/beast/http.hpp>

int main()
{
    using vix::websocket::App;
    using vix::websocket::JsonMessage;
    using vix::websocket::LongPollingBridge;
    using vix::websocket::LongPollingManager;
    using vix::websocket::Session;
    using vix::websocket::WebSocketMetrics;
    using vix::websocket::detail::ws_kvs_to_nlohmann;

    namespace http = boost::beast::http;
    using njson = nlohmann::json;

    App wsApp{"config/config.json"};
    auto &ws = wsApp.server();

    WebSocketMetrics metrics;

    std::thread metricsThread([&metrics]()
                              { vix::websocket::run_metrics_http_exporter(
                                    metrics,
                                    "0.0.0.0",
                                    9100); });
    metricsThread.detach();

    vix::websocket::SqliteMessageStore store{"chat_messages.db"};
    constexpr std::size_t HISTORY_LIMIT = 50;

    auto resolver = [](const JsonMessage &msg)
    {
        if (!msg.room.empty())
            return std::string{"room:"} + msg.room;
        return std::string{"broadcast"};
    };

    auto httpToWs = [&ws](const JsonMessage &msg)
    {
        if (!msg.room.empty())
            ws.broadcast_room_json(msg.room, msg.type, msg.payload);
        else
            ws.broadcast_json(msg.type, msg.payload);
    };

    auto lpBridge = std::make_shared<LongPollingBridge>(
        &metrics,
        std::chrono::seconds{60}, // TTL
        256,                      // max buffer / session
        resolver,
        httpToWs);

    ws.attach_long_polling_bridge(lpBridge);

    ws.on_open(
        [&store, &metrics](Session &session)
        {
            (void)session;

            metrics.connections_total.fetch_add(1, std::memory_order_relaxed);
            metrics.connections_active.fetch_add(1, std::memory_order_relaxed);

            vix::json::kvs payload{
                "user",
                "server",
                "text",
                "Welcome to Softadastra Chat 👋",
            };

            JsonMessage msg;
            msg.kind = "system";
            msg.type = "chat.system";
            msg.room = "";
            msg.payload = payload;

            store.append(msg);
            session.send_text(JsonMessage::serialize(msg));
        });

    (void)wsApp.ws(
        "/chat",
        [&ws, &store, &metrics](Session &session,
                                const std::string &type,
                                const vix::json::kvs &payload)
        {
            (void)session;

            metrics.messages_in_total.fetch_add(1, std::memory_order_relaxed);

            njson j = ws_kvs_to_nlohmann(payload);

            // 1) JOIN
            if (type == "chat.join")
            {
                std::string room = j.value("room", "");
                std::string user = j.value("user", "anonymous");

                if (!room.empty())
                {
                    ws.join_room(session, room);

                    auto history = store.list_by_room(room, HISTORY_LIMIT, std::nullopt);
                    for (auto msg : history)
                    {
                        if (msg.kind.empty())
                            msg.kind = "history";

                        session.send_text(JsonMessage::serialize(msg));
                        metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);
                    }

                    vix::json::kvs sysPayload{
                        "room",
                        room,
                        "text",
                        user + " joined the room",
                    };

                    JsonMessage sysMsg;
                    sysMsg.kind = "system";
                    sysMsg.type = "chat.system";
                    sysMsg.room = room;
                    sysMsg.payload = sysPayload;

                    store.append(sysMsg);
                    ws.broadcast_room_json(room, sysMsg.type, sysMsg.payload);
                    metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);
                }
                return;
            }

            // 2) LEAVE
            if (type == "chat.leave")
            {
                std::string room = j.value("room", "");
                std::string user = j.value("user", "anonymous");

                if (!room.empty())
                {
                    ws.leave_room(session, room);

                    vix::json::kvs sysPayload{
                        "room",
                        room,
                        "text",
                        user + " left the room",
                    };

                    JsonMessage msg;
                    msg.kind = "system";
                    msg.type = "chat.system";
                    msg.room = room;
                    msg.payload = sysPayload;

                    store.append(msg);
                    ws.broadcast_room_json(room, msg.type, msg.payload);
                    metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);
                }
                return;
            }

            // 3) MESSAGE
            if (type == "chat.message")
            {
                std::string room = j.value("room", "");
                std::string user = j.value("user", "anonymous");
                std::string text = j.value("text", "");

                if (!room.empty() && !text.empty())
                {
                    vix::json::kvs msgPayload{
                        "room",
                        room,
                        "user",
                        user,
                        "text",
                        text,
                    };

                    JsonMessage msg;
                    msg.kind = "event";
                    msg.type = "chat.message";
                    msg.room = room;
                    msg.payload = msgPayload;

                    store.append(msg);
                    ws.broadcast_room_json(room, msg.type, msg.payload);
                    metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
            }

            // 4) Fallback global
            {
                JsonMessage msg;
                msg.kind = "event";
                msg.type = type;
                msg.room = "";
                msg.payload = payload;

                store.append(msg);
                ws.broadcast_json(type, payload);
                metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);
            }
        });

    // ─────────────────────────────────────────────
    // 7) HTTP App : /ws/poll + /ws/send (LongPolling)
    // ─────────────────────────────────────────────
    vix::App httpApp;

    // Helper local pour lire ?name=value dans la target Beast
    auto get_query_param = [](const http::request<http::string_body> &req,
                              std::string_view key) -> std::optional<std::string>
    {
        std::string target = std::string(req.target());
        auto pos = target.find('?');
        if (pos == std::string::npos)
            return std::nullopt;

        std::string query = target.substr(pos + 1);
        std::size_t start = 0;

        while (start < query.size())
        {
            auto amp = query.find('&', start);
            auto part = query.substr(
                start,
                (amp == std::string::npos) ? std::string::npos : (amp - start));

            auto eq = part.find('=');
            if (eq != std::string::npos)
            {
                std::string k = part.substr(0, eq);
                std::string v = part.substr(eq + 1);
                if (k == key)
                {
                    return v;
                }
            }

            if (amp == std::string::npos)
                break;
            start = amp + 1;
        }
        return std::nullopt;
    };

    // GET /ws/poll → retourne un array JSON de JsonMessage
    httpApp.get(
        "/ws/poll",
        [lpBridge, &get_query_param](const http::request<http::string_body> &req,
                                     vix::vhttp::ResponseWrapper &res)
        {
            auto sessionIdOpt = get_query_param(req, "session_id");
            if (!sessionIdOpt || sessionIdOpt->empty())
            {
                res.status(http::status::bad_request).json({
                    "error",
                    "missing_session_id",
                });
                return;
            }

            std::string sessionId = *sessionIdOpt;

            std::size_t maxMessages = 50;
            if (auto maxStrOpt = get_query_param(req, "max"))
            {
                try
                {
                    maxMessages = static_cast<std::size_t>(std::stoul(*maxStrOpt));
                }
                catch (...)
                {
                    // on garde 50
                }
            }

            auto messages = lpBridge->poll(sessionId, maxMessages, true);
            auto body = vix::websocket::json_messages_to_nlohmann_array(messages);

            res.status(http::status::ok).json(body);
        });

    // POST /ws/send → HTTP -> LP (et via httpToWs → WS + rooms)
    httpApp.post(
        "/ws/send",
        [lpBridge](const http::request<http::string_body> &req,
                   vix::vhttp::ResponseWrapper &res)
        {
            njson j;
            try
            {
                j = njson::parse(req.body());
            }
            catch (...)
            {
                res.status(http::status::bad_request).json({
                    "error",
                    "invalid_json_body",
                });
                return;
            }

            std::string sessionId = j.value("session_id", std::string{});
            std::string type = j.value("type", std::string{});
            std::string room = j.value("room", std::string{});

            if (type.empty())
            {
                res.status(http::status::bad_request).json({
                    "error",
                    "missing_type",
                });
                return;
            }

            // Si pas de session_id fourni :
            if (sessionId.empty())
            {
                if (!room.empty())
                {
                    sessionId = std::string{"room:"} + room;
                }
                else
                {
                    sessionId = "broadcast";
                }
            }

            JsonMessage msg;
            msg.type = type;
            msg.room = room;

            if (j.contains("payload"))
            {
                msg.payload = vix::websocket::detail::nlohmann_payload_to_kvs(j["payload"]);
            }

            lpBridge->send_from_http(sessionId, msg);

            res.status(http::status::accepted).json({
                "status",
                "queued",
                "session_id",
                sessionId,
            });
        });

    // (Optionnel) /health simple
    httpApp.get(
        "/health",
        [](auto &, vix::vhttp::ResponseWrapper &res)
        {
            res.status(http::status::ok).json({
                "status",
                "ok",
            });
        });

    // ─────────────────────────────────────────────
    // 8) Lancement WS + HTTP
    // ─────────────────────────────────────────────

    // Thread dédié WebSocket
    std::thread wsThread{[&wsApp]()
                         { wsApp.run_blocking(); }};

    // Hook shutdown : quand HTTP reçoit SIGINT/SIGTERM, il sort de run()
    // et on coupe proprement le WS.
    httpApp.set_shutdown_callback([&wsApp, &wsThread]()
                                  {
        wsApp.stop();
        if (wsThread.joinable())
        {
            wsThread.join();
        } });

    // HTTP bloquant sur 8080
    httpApp.run(8080);

    return 0;
}
