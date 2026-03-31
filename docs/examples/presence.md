
# Presence (Online / Offline Users)

This example shows how to implement **presence tracking** in Vix.cpp.

Presence means:

- Knowing when a user connects
- Knowing when a user disconnects
- Broadcasting online / offline events
- Optionally tracking a simple in-memory user list

This example is beginner-friendly and uses only:

- WebSocket
- A simple std::unordered_set
- broadcast_json()

## 1) Minimal Presence Server (Single File)

main.cpp

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
#include <unordered_set>

using namespace vix;

int main()
{
  static std::unordered_set<std::string> online_users;

  vix::serve_http_and_ws([](App& app, auto& ws)
  {
    app.get("/", [](Request&, Response& res)
    {
      res.json({
        "message", "Presence example running",
        "hint", "Connect via WebSocket and send chat.join"
      });
    });

    ws.on_open([&ws](auto&)
    {
      ws.broadcast_json("presence.system", {
        "message", "A new connection opened"
      });
    });

    ws.on_typed_message([&ws](auto&, const std::string& type,
                              const vix::json::kvs& payload)
    {
      if (type == "presence.join")
      {
        std::string user;

        for (size_t i = 0; i + 1 < payload.flat.size(); i += 2)
        {
          if (std::get<std::string>(payload.flat[i].v) == "user")
          {
            user = std::get<std::string>(payload.flat[i+1].v);
          }
        }

        if (!user.empty())
        {
          online_users.insert(user);

          ws.broadcast_json("presence.online", {
            "user", user
          });
        }
      }

      if (type == "presence.leave")
      {
        std::string user;

        for (size_t i = 0; i + 1 < payload.flat.size(); i += 2)
        {
          if (std::get<std::string>(payload.flat[i].v) == "user")
          {
            user = std::get<std::string>(payload.flat[i+1].v);
          }
        }

        if (!user.empty())
        {
          online_users.erase(user);

          ws.broadcast_json("presence.offline", {
            "user", user
          });
        }
      }

      if (type == "presence.list")
      {
        std::vector<vix::json::token> arr;

        for (const auto& u : online_users)
        {
          arr.emplace_back(u);
        }

        ws.broadcast_json("presence.current", {
          "users", vix::json::array(std::move(arr))
        });
      }
    });
  });

  return 0;
}
```

## 2) How to Test (Step by Step)

### 1) Build and run:
```bash
    vix run main.cpp
```
### 2) Open two terminals.

### 3) Connect both with websocat:
```bash
    websocat ws://127.0.0.1:9090/
```
### 4) In terminal A:
```bash
    {"type":"presence.join","payload":{"user":"Alice"}}
```
### 5) In terminal B:
```bash
    {"type":"presence.join","payload":{"user":"Bob"}}
```
Both terminals will receive:
```bash
    presence.online
```
### 6) Request current list:
```bash
    {"type":"presence.list","payload":{}}
```
You will receive:
```bash
    presence.current
```
### 7) Leave:
```bash
    {"type":"presence.leave","payload":{"user":"Alice"}}
```
## 3) What This Teaches

- How to react to connection events
- How to track state in memory
- How to broadcast presence changes
- How to implement basic real-time features

## 4) Production Notes

For real systems:

- Store presence in Redis or a distributed cache
- Use heartbeat timeouts to detect disconnects
- Clean users on connection close (ws.on_close)
- Avoid trusting user-provided names without authentication

Presence is one of the most common real-time patterns:
chat apps, dashboards, multiplayer games, admin panels.

