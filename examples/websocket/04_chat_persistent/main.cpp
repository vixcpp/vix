/**
 *
 *  @file examples/websocket/04_chat_persistent/main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  Persistent chat example.
 *  Goal:
 *    - run HTTP + WebSocket together
 *    - persist chat messages into SQLite
 *    - expose room history through HTTP
 *    - keep the example structured and production-like
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
#include <vix/websocket/AttachedRuntime.hpp>
#include <vix/websocket/SqliteMessageStore.hpp>

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
        remove_from_room_unlocked(session_id, it->second);
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

      remove_from_room_unlocked(session_id, it->second);
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

    [[nodiscard]] std::size_t room_count() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return rooms_.size();
    }

    [[nodiscard]] std::size_t member_count() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return members_by_session_.size();
    }

  private:
    void remove_from_room_unlocked(
        const std::string &session_id,
        const ChatMember &member)
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

  /**
   * @brief Shared state for the persistent chat example.
   */
  struct ChatPersistentState
  {
    ChatRoomRegistry registry;
    vix::websocket::SqliteMessageStore store{"storage/chat_rooms.db"};
  };

  /**
   * @brief Facade that owns the runtime objects.
   */
  struct ChatPersistentRuntime
  {
    vix::config::Config config{".env"};
    std::shared_ptr<vix::executor::RuntimeExecutor> executor{
        std::make_shared<vix::executor::RuntimeExecutor>(1u)};
    vix::App app{executor};
    vix::websocket::Server ws{config, executor};
    std::shared_ptr<ChatPersistentState> state{std::make_shared<ChatPersistentState>()};
  };

  [[nodiscard]] std::string session_key(vix::websocket::Session &session)
  {
    const void *raw = static_cast<const void *>(&session);
    return std::to_string(reinterpret_cast<std::uintptr_t>(raw));
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

  void persist_event(
      ChatPersistentState &state,
      const std::string &room,
      const std::string &type,
      const vix::json::kvs &payload,
      const std::string &kind = "event")
  {
    vix::websocket::JsonMessage msg;
    msg.room = room;
    msg.type = type;
    msg.kind = kind;
    msg.payload = payload;

    state.store.append(msg);
  }

  void emit_system_message(
      ChatPersistentState &state,
      vix::websocket::Server &ws,
      const std::string &room,
      const std::string &text,
      const std::string &user)
  {
    const vix::json::kvs payload{
        "room",
        room,
        "user",
        user,
        "text",
        text,
    };

    persist_event(state, room, "chat.system", payload);
    ws.broadcast_json("chat.system", payload);
  }

  void handle_join(
      ChatPersistentState &state,
      vix::websocket::Server &ws,
      vix::websocket::Session &session,
      const vix::json::kvs &payload)
  {
    const nlohmann::json json_payload =
        vix::websocket::detail::ws_kvs_to_nlohmann(payload);

    const std::string user = json_string_or(json_payload, "user", "anonymous");
    const std::string room = json_string_or(json_payload, "room", "general");
    const std::string sid = session_key(session);

    state.registry.join(sid, user, room);

    const vix::json::kvs joined_payload{
        "room",
        room,
        "user",
        user,
    };

    persist_event(state, room, "chat.joined", joined_payload);
    ws.broadcast_json("chat.joined", joined_payload);

    emit_system_message(state, ws, room, user + " joined the room", user);
  }

  void handle_leave(
      ChatPersistentState &state,
      vix::websocket::Server &ws,
      vix::websocket::Session &session)
  {
    const std::string sid = session_key(session);
    const ChatMember member = state.registry.member_of(sid);

    if (member.room.empty())
    {
      return;
    }

    state.registry.leave(sid);

    const vix::json::kvs left_payload{
        "room",
        member.room,
        "user",
        member.user,
    };

    persist_event(state, member.room, "chat.left", left_payload);
    ws.broadcast_json("chat.left", left_payload);

    emit_system_message(
        state,
        ws,
        member.room,
        member.user + " left the room",
        member.user);
  }

  void handle_message(
      ChatPersistentState &state,
      vix::websocket::Server &ws,
      vix::websocket::Session &session,
      const vix::json::kvs &payload)
  {
    const std::string sid = session_key(session);
    const ChatMember member = state.registry.member_of(sid);

    if (member.room.empty())
    {
      ws.broadcast_json(
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

    const vix::json::kvs out_payload{
        "room",
        member.room,
        "user",
        member.user,
        "text",
        text,
    };

    persist_event(state, member.room, "chat.message", out_payload, "message");
    ws.broadcast_json("chat.message", out_payload);
  }

  void handle_typing(
      ChatPersistentState &state,
      vix::websocket::Server &ws,
      vix::websocket::Session &session)
  {
    const std::string sid = session_key(session);
    const ChatMember member = state.registry.member_of(sid);

    if (member.room.empty())
    {
      return;
    }

    const vix::json::kvs payload{
        "room",
        member.room,
        "user",
        member.user,
    };

    persist_event(state, member.room, "chat.typing", payload);
    ws.broadcast_json("chat.typing", payload);
  }

  void register_http_routes(ChatPersistentRuntime &runtime)
  {
    runtime.app.get(
        "/",
        [](vix::Request &, vix::Response &res)
        {
          res.json({
              {"name", "Vix Persistent Chat"},
              {"message", "HTTP + WebSocket + SQLite history"},
              {"framework", "Vix.cpp"},
          });
        });

    runtime.app.get(
        "/health",
        [](vix::Request &, vix::Response &res)
        {
          res.json({
              {"status", "ok"},
              {"service", "chat-persistent"},
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
              {"total", static_cast<int>(rooms.size())},
          });
        });

    runtime.app.get(
        "/rooms/{room}",
        [state = runtime.state](vix::Request &req, vix::Response &res)
        {
          const std::string room = req.param("room", "general");
          const auto users = state->registry.users_in_room(room);

          nlohmann::json members = nlohmann::json::array();
          for (const auto &user : users)
          {
            members.push_back(user);
          }

          res.json({
              {"room", room},
              {"users", members},
              {"count", static_cast<int>(users.size())},
          });
        });

    runtime.app.get(
        "/rooms/{room}/messages",
        [state = runtime.state](vix::Request &req, vix::Response &res)
        {
          const std::string room = req.param("room", "general");

          const int limit =
              parse_positive_int_or(req.query_value("limit", "50"), 50);

          const std::string before_id =
              req.query_value("before_id", "");

          const std::optional<std::string> cursor =
              before_id.empty()
                  ? std::nullopt
                  : std::optional<std::string>(before_id);

          const auto messages =
              state->store.list_by_room(
                  room,
                  static_cast<std::size_t>(limit),
                  cursor);

          nlohmann::json items = nlohmann::json::array();
          for (const auto &msg : messages)
          {
            items.push_back(to_json(msg));
          }

          res.json({
              {"room", room},
              {"messages", items},
              {"count", static_cast<int>(items.size())},
          });
        });

    runtime.app.get(
        "/replay/{start_id}",
        [state = runtime.state](vix::Request &req, vix::Response &res)
        {
          const std::string start_id = req.param("start_id", "");
          const int limit = parse_positive_int_or(req.query_value("limit", "50"), 50);

          const auto messages =
              state->store.replay_from(start_id, static_cast<std::size_t>(limit));

          nlohmann::json items = nlohmann::json::array();
          for (const auto &msg : messages)
          {
            items.push_back(to_json(msg));
          }

          res.json({
              {"start_id", start_id},
              {"messages", items},
              {"count", static_cast<int>(items.size())},
          });
        });

    runtime.app.get(
        "/stats",
        [state = runtime.state](vix::Request &, vix::Response &res)
        {
          res.json({
              {"rooms", static_cast<int>(state->registry.room_count())},
              {"members", static_cast<int>(state->registry.member_count())},
          });
        });
  }

  void register_ws_lifecycle(ChatPersistentRuntime &runtime)
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
        [state = runtime.state, &ws = runtime.ws](vix::websocket::Session &session)
        {
          handle_leave(*state, ws, session);
        });

    runtime.ws.on_error(
        [](vix::websocket::Session &session, const std::string &error)
        {
          (void)session;
          (void)error;
        });
  }

  void register_ws_protocol(ChatPersistentRuntime &runtime)
  {
    runtime.ws.on_typed_message(
        [state = runtime.state, &ws = runtime.ws](
            vix::websocket::Session &session,
            const std::string &type,
            const vix::json::kvs &payload)
        {
          if (type == "chat.join")
          {
            handle_join(*state, ws, session, payload);
            return;
          }

          if (type == "chat.leave")
          {
            handle_leave(*state, ws, session);
            return;
          }

          if (type == "chat.message")
          {
            handle_message(*state, ws, session, payload);
            return;
          }

          if (type == "chat.typing")
          {
            handle_typing(*state, ws, session);
            return;
          }

          ws.broadcast_json(
              "chat.unknown",
              {
                  "type",
                  type,
                  "message",
                  "Unknown chat event type",
              });
        });
  }

  void configure(ChatPersistentRuntime &runtime)
  {
    register_http_routes(runtime);
    register_ws_lifecycle(runtime);
    register_ws_protocol(runtime);
  }

  void run(ChatPersistentRuntime &runtime)
  {
    const int http_port = runtime.config.getServerPort();
    vix::run_http_and_ws(runtime.app, runtime.ws, runtime.executor, http_port);
  }
} // namespace

int main()
{
  ChatPersistentRuntime runtime;
  configure(runtime);
  run(runtime);
  return 0;
}
