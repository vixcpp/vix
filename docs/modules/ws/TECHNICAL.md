# Vix WebSocket - Full Documentation

Typed, room-based, persistent WebSocket runtime for C++20.

---

# Features

- Async Beast/Asio WebSocket server  
- Rooms (join/leave/broadcast)  
- SQLite storage (WAL)  
- Typed JSON protocol  
- Replay by ID  
- Long Polling fallback  
- C++ and browser clients  
- Metrics-ready  

---

# Quick Start

```cpp
vix::websocket::Server ws(cfg, exec);
ws.listen_blocking();
```

---

# Event Handling

```cpp
ws.on_open([](Session& s){
    s.send_text("Welcome!");
});
```

```cpp
ws.on_typed_message([](Session& s, const std::string&t, const kvs&p){
    if(t=="chat.message"){}
});
```

---

# Rooms

```cpp
ws.join_room(session, "africa");
ws.broadcast_room_json("africa", "chat.message", {"text", "Hello!"});
```

---

# Storage

```cpp
SqliteMessageStore store{"chat.db"};
store.append("chat", "africa", "chat.message", payload);
```

---

# Replay

```cpp
auto msgs = store.list_by_room("africa", 50, std::nullopt);
```

---

# Clients

See CLIENT_GUIDE.md

---

# Roadmap

- Presence  
- Encryption  
- Batch messages  
- Binary frames  
- Auto reconnect  

---

MIT License
