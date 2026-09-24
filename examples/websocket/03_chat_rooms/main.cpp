/**
 *
 *  @file examples/websocket/03_chat_rooms/main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  Chat rooms example.
 *  Goal:
 *    - run HTTP + WebSocket together
 *    - manage simple in-memory chat rooms
 *    - handle join / leave / message / typing events
 *    - expose small HTTP endpoints for inspection
 *
 */

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

namespace
{
  struct ChatMember
  {
    std::string user;
    std::string room;
  };

  /**
   * @brief In-memory registry for room membership.
   *
   * This is intentionally simple for the example:
   * - maps session id -> current room/user
   * - tracks room -> user list
   */
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

    [[nodiscard]] bool has_member(const std::string &session_id) const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return members_by_session_.find(session_id) != members_by_session_.end();
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
   * @brief Shared application state for the example.
   */
  struct ChatRoomsState
  {
    ChatRoomRegistry registry;
  };

  /**
   * @brief Small facade that owns the runtime pieces.
   */
  struct ChatRoomsRuntime
  {
    vix::config::Config config{".env"};
    std::shared_ptr<vix::executor::RuntimeExecutor> executor{
        std::make_shared<vix::executor::RuntimeExecutor>(1u)};
    vix::App app{executor};
    vix::websocket::Server ws{config, executor};
    std::shared_ptr<ChatRoomsState> state{std::make_shared<ChatRoomsState>()};
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

  void emit_system_message(
      vix::websocket::Server &ws,
      const std::string &room,
      const std::string &text,
      const std::string &user)
  {
    ws.broadcast_json(
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
      ChatRoomsState &state,
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

    emit_system_message(
        ws,
        room,
        user + " joined the room",
        user);

    ws.broadcast_json(
        "chat.joined",
        {
            "room",
            room,
            "user",
            user,
        });
  }

  void handle_leave(
      ChatRoomsState &state,
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

    emit_system_message(
        ws,
        member.room,
        member.user + " left the room",
        member.user);

    ws.broadcast_json(
        "chat.left",
        {
            "room",
            member.room,
            "user",
            member.user,
        });
  }

  void handle_message(
      ChatRoomsState &state,
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

    ws.broadcast_json(
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
      ChatRoomsState &state,
      vix::websocket::Server &ws,
      vix::websocket::Session &session)
  {
    const std::string sid = session_key(session);
    const ChatMember member = state.registry.member_of(sid);

    if (member.room.empty())
    {
      return;
    }

    ws.broadcast_json(
        "chat.typing",
        {
            "room",
            member.room,
            "user",
            member.user,
        });
  }

  void register_http_routes(ChatRoomsRuntime &runtime)
  {
    runtime.app.get(
        "/",
        [](vix::Request &, vix::Response &res)
        {
          res.json({
              {"name", "Vix Chat Rooms"},
              {"message", "HTTP + WebSocket chat rooms example"},
              {"framework", "Vix.cpp"},
          });
        });

    runtime.app.get(
        "/health",
        [](vix::Request &, vix::Response &res)
        {
          res.json({
              {"status", "ok"},
              {"service", "chat-rooms"},
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
        "/stats",
        [state = runtime.state](vix::Request &, vix::Response &res)
        {
          res.json({
              {"rooms", static_cast<int>(state->registry.room_count())},
              {"members", static_cast<int>(state->registry.member_count())},
          });
        });
  }

  void register_ws_lifecycle(ChatRoomsRuntime &runtime)
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

  void register_ws_protocol(ChatRoomsRuntime &runtime)
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

  void configure(ChatRoomsRuntime &runtime)
  {
    register_http_routes(runtime);
    register_ws_lifecycle(runtime);
    register_ws_protocol(runtime);
  }

  void run(ChatRoomsRuntime &runtime)
  {
    const int http_port = runtime.config.getServerPort();
    vix::run_http_and_ws(runtime.app, runtime.ws, runtime.executor, http_port);
  }
} // namespace

int main()
{
  ChatRoomsRuntime runtime;
  configure(runtime);
  run(runtime);
  return 0;
}
