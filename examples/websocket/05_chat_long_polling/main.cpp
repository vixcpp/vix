/**
 *
 *  @file examples/websocket/05_chat_long_polling/main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  Chat with long-polling fallback.
 *  Goal:
 *    - run HTTP + WebSocket together
 *    - support room-based chat over WebSocket
 *    - expose HTTP long-polling fallback routes
 *    - keep one shared event model for WS and HTTP clients
 *
 */

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

#include <vix.hpp>
#include <vix/websocket.hpp>

namespace
{
  struct ChatMember
  {
    std::string user;
    std::string room;
  };

  class ChatRoomRegistry
  {
  public:
    void join(
        const std::string &session_id,
        std::string user,
        std::string room)
    {
      std::lock_guard<std::mutex> lock(mutex_);

      auto it = members_by_session_.find(session_id);
      if (it != members_by_session_.end())
      {
        remove_unlocked(session_id, it->second);
      }

      ChatMember member{std::move(user), std::move(room)};
      rooms_[member.room].insert(member.user);
      members_by_session_[session_id] = std::move(member);
    }

    void leave(const std::string &session_id)
    {
      std::lock_guard<std::mutex> lock(mutex_);

      auto it = members_by_session_.find(session_id);
      if (it == members_by_session_.end())
      {
        return;
      }

      remove_unlocked(session_id, it->second);
    }

    [[nodiscard]] ChatMember member_of(const std::string &session_id) const
    {
      std::lock_guard<std::mutex> lock(mutex_);

      auto it = members_by_session_.find(session_id);
      if (it == members_by_session_.end())
      {
        return {};
      }

      return it->second;
    }

    [[nodiscard]] std::vector<std::string> room_names() const
    {
      std::lock_guard<std::mutex> lock(mutex_);

      std::vector<std::string> out;
      out.reserve(rooms_.size());

      for (const auto &[room, _] : rooms_)
      {
        out.push_back(room);
      }

      return out;
    }

    [[nodiscard]] std::vector<std::string> users_in_room(const std::string &room) const
    {
      std::lock_guard<std::mutex> lock(mutex_);

      std::vector<std::string> out;
      auto it = rooms_.find(room);
      if (it == rooms_.end())
      {
        return out;
      }

      out.reserve(it->second.size());
      for (const auto &user : it->second)
      {
        out.push_back(user);
      }

      return out;
    }

  private:
    void remove_unlocked(const std::string &session_id, const ChatMember &member)
    {
      auto room_it = rooms_.find(member.room);
      if (room_it != rooms_.end())
      {
        room_it->second.erase(member.user);
        if (room_it->second.empty())
        {
          rooms_.erase(room_it);
        }
      }

      members_by_session_.erase(session_id);
    }

  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ChatMember> members_by_session_;
    std::unordered_map<std::string, std::unordered_set<std::string>> rooms_;
  };

  struct ChatLongPollingState
  {
    ChatRoomRegistry registry;
  };

  struct ChatLongPollingRuntime
  {
    vix::config::Config config{".env"};
    std::shared_ptr<vix::executor::RuntimeExecutor> executor{
        std::make_shared<vix::executor::RuntimeExecutor>(1u)};
    vix::App app{executor};
    vix::websocket::Server ws{config, executor};
    std::shared_ptr<ChatLongPollingState> state{
        std::make_shared<ChatLongPollingState>()};
    std::shared_ptr<vix::websocket::LongPollingBridge> bridge{};
  };

  [[nodiscard]] std::string ws_session_key(vix::websocket::Session &session)
  {
    const void *raw = static_cast<const void *>(&session);
    return std::string{"ws:"} + std::to_string(reinterpret_cast<std::uintptr_t>(raw));
  }

  [[nodiscard]] std::string room_session_id(const std::string &room)
  {
    return std::string{"room:"} + room;
  }

  [[nodiscard]] std::string json_string_or(
      const nlohmann::json &j,
      const char *key,
      const std::string &fallback)
  {
    if (!j.is_object())
    {
      return fallback;
    }

    auto it = j.find(key);
    if (it == j.end() || !it->is_string())
    {
      return fallback;
    }

    return it->get<std::string>();
  }

  [[nodiscard]] int parse_positive_int_or(const std::string &s, int fallback)
  {
    if (s.empty())
    {
      return fallback;
    }

    try
    {
      const int value = std::stoi(s);
      return value > 0 ? value : fallback;
    }
    catch (...)
    {
      return fallback;
    }
  }

  [[nodiscard]] nlohmann::json to_json(const vix::websocket::JsonMessage &msg)
  {
    return nlohmann::json{
        {"id", msg.id},
        {"kind", msg.kind},
        {"room", msg.room},
        {"type", msg.type},
        {"ts", msg.ts},
        {"payload", vix::websocket::detail::ws_kvs_to_nlohmann(msg.payload)},
    };
  }

  void configure_long_polling_bridge(ChatLongPollingRuntime &runtime)
  {
    auto resolver = [](const vix::websocket::JsonMessage &msg) -> std::string
    {
      if (!msg.room.empty())
      {
        return room_session_id(msg.room);
      }

      return "broadcast";
    };

    auto http_to_ws = [&ws = runtime.ws](const vix::websocket::JsonMessage &msg)
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

    runtime.bridge = std::make_shared<vix::websocket::LongPollingBridge>(
        nullptr,
        std::chrono::seconds{60},
        256,
        resolver,
        http_to_ws);

    runtime.ws.attach_long_polling_bridge(runtime.bridge);
  }

  void emit_room_system(
      vix::websocket::Server &ws,
      const std::string &room,
      const std::string &user,
      const std::string &text)
  {
    ws.broadcast_room_json(
        room,
        "chat.system",
        {
            "room",
            room,
            "user",
            user,
            "text",
            text,
        });
  }

  void handle_join(
      ChatLongPollingRuntime &runtime,
      vix::websocket::Session &session,
      const vix::json::kvs &payload)
  {
    const nlohmann::json json_payload =
        vix::websocket::detail::ws_kvs_to_nlohmann(payload);

    const std::string user = json_string_or(json_payload, "user", "anonymous");
    const std::string room = json_string_or(json_payload, "room", "general");
    const std::string sid = ws_session_key(session);

    runtime.state->registry.join(sid, user, room);
    runtime.ws.join_room(session, room);

    runtime.ws.broadcast_room_json(
        room,
        "chat.joined",
        {
            "room",
            room,
            "user",
            user,
            "lp_session_id",
            room_session_id(room),
        });

    emit_room_system(runtime.ws, room, user, user + " joined the room");
  }

  void handle_leave(
      ChatLongPollingRuntime &runtime,
      vix::websocket::Session &session)
  {
    const std::string sid = ws_session_key(session);
    const ChatMember member = runtime.state->registry.member_of(sid);

    if (member.room.empty())
    {
      runtime.ws.leave_all_rooms(session);
      return;
    }

    runtime.state->registry.leave(sid);
    runtime.ws.leave_room(session, member.room);

    runtime.ws.broadcast_room_json(
        member.room,
        "chat.left",
        {
            "room",
            member.room,
            "user",
            member.user,
        });

    emit_room_system(
        runtime.ws,
        member.room,
        member.user,
        member.user + " left the room");
  }

  void handle_message(
      ChatLongPollingRuntime &runtime,
      vix::websocket::Session &session,
      const vix::json::kvs &payload)
  {
    const std::string sid = ws_session_key(session);
    const ChatMember member = runtime.state->registry.member_of(sid);

    if (member.room.empty())
    {
      runtime.ws.broadcast_json(
          "chat.error",
          {
              "error",
              "join a room before sending messages",
          });
      return;
    }

    const nlohmann::json json_payload =
        vix::websocket::detail::ws_kvs_to_nlohmann(payload);

    const std::string text = json_string_or(json_payload, "text", "");

    runtime.ws.broadcast_room_json(
        member.room,
        "chat.message",
        {
            "room",
            member.room,
            "user",
            member.user,
            "text",
            text,
        });
  }

  void handle_typing(
      ChatLongPollingRuntime &runtime,
      vix::websocket::Session &session)
  {
    const std::string sid = ws_session_key(session);
    const ChatMember member = runtime.state->registry.member_of(sid);

    if (member.room.empty())
    {
      return;
    }

    runtime.ws.broadcast_room_json(
        member.room,
        "chat.typing",
        {
            "room",
            member.room,
            "user",
            member.user,
        });
  }

  static vix::json::token json_to_simple_token(const nlohmann::json &value)
  {
    using namespace vix::json;

    if (value.is_null())
    {
      return token(nullptr);
    }

    if (value.is_boolean())
    {
      return token(value.get<bool>());
    }

    if (value.is_number_integer())
    {
      return token(value.get<std::int64_t>());
    }

    if (value.is_number_unsigned())
    {
      return token(static_cast<std::int64_t>(value.get<std::uint64_t>()));
    }

    if (value.is_number_float())
    {
      return token(value.get<double>());
    }

    if (value.is_string())
    {
      return token(value.get<std::string>());
    }

    if (value.is_array())
    {
      vix::json::array_t arr;
      arr.reserve(value.size());

      for (const auto &item : value)
      {
        arr.push_back(json_to_simple_token(item));
      }

      return token(arr);
    }

    if (value.is_object())
    {
      vix::json::kvs out;

      for (auto it = value.begin(); it != value.end(); ++it)
      {
        out.push_pair(
            vix::json::token(it.key()),
            json_to_simple_token(it.value()));
      }

      return vix::json::token(out);
    }

    return token(nullptr);
  }

  static vix::json::kvs json_object_to_kvs(const nlohmann::json &value)
  {
    if (!value.is_object())
    {
      return vix::json::kvs{};
    }

    vix::json::kvs out;

    for (auto it = value.begin(); it != value.end(); ++it)
    {
      out.push_pair(
          vix::json::token(it.key()),
          json_to_simple_token(it.value()));
    }

    return out;
  }

  void register_http_routes(ChatLongPollingRuntime &runtime)
  {
    runtime.app.get(
        "/",
        [](vix::Request &, vix::Response &res)
        {
          res.json({
              {"name", "Vix Chat Long Polling"},
              {"message", "WebSocket chat with HTTP long-poll fallback"},
              {"framework", "Vix.cpp"},
          });
        });

    runtime.app.get(
        "/health",
        [](vix::Request &, vix::Response &res)
        {
          res.json({
              {"status", "ok"},
              {"service", "chat-long-polling"},
          });
        });

    runtime.app.get(
        "/rooms",
        [state = runtime.state](vix::Request &, vix::Response &res)
        {
          const auto rooms = state->registry.room_names();

          nlohmann::json items = nlohmann::json::array();
          for (const auto &room : rooms)
          {
            items.push_back(room);
          }

          res.json({
              {"rooms", items},
              {"total", static_cast<int>(items.size())},
          });
        });

    runtime.app.get(
        "/rooms/{room}",
        [state = runtime.state](vix::Request &req, vix::Response &res)
        {
          const std::string room = req.param("room", "general");
          const auto users = state->registry.users_in_room(room);

          nlohmann::json items = nlohmann::json::array();
          for (const auto &user : users)
          {
            items.push_back(user);
          }

          res.json({
              {"room", room},
              {"users", items},
              {"count", static_cast<int>(items.size())},
              {"lp_session_id", room_session_id(room)},
          });
        });

    runtime.app.get(
        "/ws/poll",
        [&ws = runtime.ws](vix::Request &req, vix::Response &res)
        {
          auto bridge = ws.long_polling_bridge();
          if (!bridge)
          {
            res.status(503).json(vix::json::kv({
                {"error", "long-polling bridge not attached"},
            }));
            return;
          }

          std::string session_id = req.query_value("session_id", "broadcast");
          const int max = parse_positive_int_or(req.query_value("max", "50"), 50);

          const auto messages = bridge->poll(
              session_id,
              static_cast<std::size_t>(max),
              true);

          nlohmann::json items = nlohmann::json::array();
          for (const auto &msg : messages)
          {
            items.push_back(to_json(msg));
          }

          res.json({
              {"session_id", session_id},
              {"messages", items},
              {"count", static_cast<int>(items.size())},
          });
        });

    runtime.app.post(
        "/ws/send",
        [&ws = runtime.ws](vix::Request &req, vix::Response &res)
        {
          auto bridge = ws.long_polling_bridge();
          if (!bridge)
          {
            res.status(503).json(vix::json::kv({
                {"error", "long-polling bridge not attached"},
            }));
            return;
          }

          nlohmann::json body;
          try
          {
            body = req.json();
          }
          catch (const std::exception &)
          {
            res.status(400).json(vix::json::kv({
                {"error", "invalid JSON body"},
            }));
            return;
          }

          if (!body.is_object())
          {
            res.status(400).json(vix::json::kv({
                {"error", "JSON body must be an object"},
            }));
            return;
          }

          const std::string type = json_string_or(body, "type", "");
          const std::string room = json_string_or(body, "room", "");
          const std::string session_id =
              json_string_or(body, "session_id", room.empty() ? "broadcast" : room_session_id(room));

          if (type.empty())
          {
            res.status(400).json(vix::json::kv({
                {"error", "missing type"},
            }));
            return;
          }

          vix::websocket::JsonMessage msg;
          msg.type = type;
          msg.room = room;
          msg.kind = "event";

          if (auto it = body.find("payload"); it != body.end() && it->is_object())
          {
            msg.payload = json_object_to_kvs(*it);
          }
          else
          {
            msg.payload = vix::json::kvs{};
          }

          bridge->send_from_http(session_id, msg);

          res.status(202).json({
              {"status", "queued"},
              {"session_id", session_id},
              {"type", type},
              {"room", room},
          });
        });
  }

  void register_ws_lifecycle(ChatLongPollingRuntime &runtime)
  {
    runtime.ws.on_open(
        [&ws = runtime.ws](vix::websocket::Session &session)
        {
          (void)session;

          ws.broadcast_json(
              "system.connected",
              {
                  "message",
                  "A WebSocket client connected",
              });
        });

    runtime.ws.on_close(
        [&runtime](vix::websocket::Session &session)
        {
          handle_leave(runtime, session);
        });

    runtime.ws.on_error(
        [](vix::websocket::Session &session, const std::string &error)
        {
          (void)session;
          (void)error;
        });
  }

  void register_ws_protocol(ChatLongPollingRuntime &runtime)
  {
    runtime.ws.on_typed_message(
        [&runtime](
            vix::websocket::Session &session,
            const std::string &type,
            const vix::json::kvs &payload)
        {
          if (type == "chat.join")
          {
            handle_join(runtime, session, payload);
            return;
          }

          if (type == "chat.leave")
          {
            handle_leave(runtime, session);
            return;
          }

          if (type == "chat.message")
          {
            handle_message(runtime, session, payload);
            return;
          }

          if (type == "chat.typing")
          {
            handle_typing(runtime, session);
            return;
          }

          runtime.ws.broadcast_json(
              "chat.unknown",
              {
                  "type",
                  type,
                  "message",
                  "Unknown chat event type",
              });
        });
  }

  void configure(ChatLongPollingRuntime &runtime)
  {
    configure_long_polling_bridge(runtime);
    register_http_routes(runtime);
    register_ws_lifecycle(runtime);
    register_ws_protocol(runtime);
  }

  void run(ChatLongPollingRuntime &runtime)
  {
    const int http_port = runtime.config.getServerPort();
    vix::run_http_and_ws(runtime.app, runtime.ws, runtime.executor, http_port);
  }
} // namespace

int main()
{
  ChatLongPollingRuntime runtime;
  configure(runtime);
  run(runtime);
  return 0;
}
