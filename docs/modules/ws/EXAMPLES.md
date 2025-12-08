# Vix WebSocket - Advanced Examples

This document provides advanced examples for building real-time systems using the Vix WebSocket module.

---

## 1. Advanced Room Chat Server

```cpp
#include <vix/websocket.hpp>
#include <vix/config/Config.hpp>
#include <nlohmann/json.hpp>

using namespace vix;
using namespace vix::websocket;

int main() {
    config::Config cfg{"config/config.json"};
    auto exec = experimental::make_threadpool_executor(4, 8, 0);

    Server ws(cfg, exec);
    SqliteMessageStore store{"chat_messages.db"};

    ws.on_open([&](Session& s){
        ws.send_json(s,
            JsonMessage::serialize("chat.system",
                {"text", "Welcome to Advanced Chat!"}
            )
        );
    });

    ws.on_typed_message([&](Session& s, const std::string &type, const kvs &payload){
        auto j = ws_kvs_to_nlohmann(payload);

        if (type == "chat.join") {
            std::string room = j.value("room", "");
            ws.join_room(s, room);

            ws.broadcast_room_json(room, "chat.system",
                {"text", j["user"].get<std::string>() + " joined"}
            );
        }

        else if (type == "chat.message") {
            std::string room = j.value("room", "");
            store.append("chat", room, type, j);

            ws.broadcast_room_json(room, "chat.message", {
                "user", j["user"],
                "room", room,
                "text", j["text"]
            });
        }
    });

    ws.listen_blocking();
}
```

---

## 2. Fetching History

```cpp
auto history = store.list_by_room(room, 50, std::nullopt);
for (auto &msg : history) ws.send_json(session, msg.to_json());
```

---

## 3. Global Announcement

```cpp
ws.broadcast_all_json("server.notice", {"text", "Maintenance soon"});
```

---

## 4. Remove Client From Rooms

```cpp
ws.on_close([&](Session& s){ ws.remove_from_all_rooms(s); });
```
