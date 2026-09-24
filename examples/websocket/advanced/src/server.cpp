/**
 *
 *  @file server.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 * This example demonstrates a fully featured, production-style WebSocket
 * server using the Vix.cpp runtime. It showcases how to combine:
 *
 *  • Asynchronous native WebSocket server
 *  • RuntimeExecutor integration
 *  • Room-based messaging (join, leave, broadcast)
 *  • Typed JSON protocol ("type" + "payload")
 *  • Persistent message storage using SQLite (WAL enabled)
 *  • Automatic replay of chat history on join
 *  • Prometheus-compatible metrics server (/metrics endpoint)
 *  • Structured system events for room lifecycle (join/leave)
 *
 */
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include <nlohmann/json.hpp>

#include <vix.hpp>

#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/websocket.hpp>
#include <vix/websocket/LongPolling.hpp>
#include <vix/websocket/LongPollingBridge.hpp>
#include <vix/websocket/Metrics.hpp>
#include <vix/websocket/SqliteMessageStore.hpp>
#include <vix/websocket/protocol.hpp>

int main()
{
  using vix::websocket::App;
  using vix::websocket::JsonMessage;
  using vix::websocket::LongPollingBridge;
  using vix::websocket::LongPollingManager;
  using vix::websocket::Session;
  using vix::websocket::WebSocketMetrics;
  using vix::websocket::detail::ws_kvs_to_nlohmann;

  using njson = nlohmann::json;

  auto exec = std::make_shared<vix::executor::RuntimeExecutor>();

  App wsApp{".env", exec};
  auto &ws = wsApp.server();

  WebSocketMetrics metrics;

  std::thread metricsThread(
      [&metrics]()
      {
        vix::websocket::run_metrics_http_exporter(
            metrics,
            "0.0.0.0",
            9100);
      });
  metricsThread.detach();

  vix::websocket::SqliteMessageStore store{"chat_messages.db"};
  constexpr std::size_t HISTORY_LIMIT = 50;

  auto resolver = [](const JsonMessage &msg)
  {
    if (!msg.room.empty())
    {
      return std::string{"room:"} + msg.room;
    }
    return std::string{"broadcast"};
  };

  auto httpToWs = [&ws](const JsonMessage &msg)
  {
    if (!msg.room.empty())
    {
      ws.broadcast_room_json(msg.room, msg.type, msg.payload);
    }
    else
    {
      ws.broadcast_json(msg.type, msg.payload);
    }
  };

  auto lpBridge = std::make_shared<LongPollingBridge>(
      &metrics,
      std::chrono::seconds{60},
      256,
      resolver,
      httpToWs);

  ws.attach_long_polling_bridge(lpBridge);

  ws.on_open(
      [&store, &metrics](Session &session)
      {
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
        metrics.messages_in_total.fetch_add(1, std::memory_order_relaxed);

        njson j = ws_kvs_to_nlohmann(payload);

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
              {
                msg.kind = "history";
              }

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

  vix::App httpApp;

  httpApp.get(
      "/ws/poll",
      [lpBridge](vix::http::Request &req,
                 vix::http::ResponseWrapper &res)
      {
        const std::string sessionId = req.query_value("session_id");
        if (sessionId.empty())
        {
          res.bad_request().json(nlohmann::json{
              {"error", "missing_session_id"},
          });
          return;
        }

        std::size_t maxMessages = 50;
        if (req.has_query("max"))
        {
          try
          {
            maxMessages = static_cast<std::size_t>(
                std::stoul(req.query_value("max")));
          }
          catch (...)
          {
          }
        }

        auto messages = lpBridge->poll(sessionId, maxMessages, true);
        auto body = vix::websocket::json_messages_to_nlohmann_array(messages);

        res.ok().json(body);
      });

  httpApp.post(
      "/ws/send",
      [lpBridge](vix::http::Request &req,
                 vix::http::ResponseWrapper &res)
      {
        njson j;
        try
        {
          j = njson::parse(req.body());
        }
        catch (...)
        {
          res.bad_request().json(njson{
              {"error", "invalid_json_body"},
          });
          return;
        }

        std::string sessionId = j.value("session_id", std::string{});
        std::string type = j.value("type", std::string{});
        std::string room = j.value("room", std::string{});

        if (type.empty())
        {
          res.bad_request().json(njson{
              {"error", "missing_type"},
          });
          return;
        }

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

        res.status(202).json(njson{
            {"status", "queued"},
            {"session_id", sessionId},
        });
      });

  httpApp.get(
      "/health",
      [](vix::http::Request &, vix::http::ResponseWrapper &res)
      {
        res.ok().json(njson{
            {"status", "ok"},
        });
      });

  std::thread wsThread(
      [&wsApp]()
      {
        wsApp.run_blocking();
      });

  httpApp.set_shutdown_callback(
      [&wsApp, &wsThread]()
      {
        wsApp.stop();
        if (wsThread.joinable())
        {
          wsThread.join();
        }
      });

  httpApp.run(8080);

  return 0;
}
