# Vix WebSocket - Client Guide

This guide explains how to connect to Vix WebSocket servers from the browser and C++.

---

# 1. Browser WebSocket Client

```js
const ws = new WebSocket("ws://localhost:9090/");

ws.onopen = () => {
  ws.send(JSON.stringify({
    type: "chat.join",
    payload: { user: "Alice", room: "africa" }
  }));
};

ws.onmessage = (e) => console.log("Received:", e.data);
```

---

## Sending a message

```js
ws.send(JSON.stringify({
  type: "chat.message",
  payload: { user: "Alice", room: "africa", text: "Hello!" }
}));
```

---

# 2. Browser Long Polling Fallback

```js
async function poll() {
  const res = await fetch("/ws/poll?session_id=123");
  const data = await res.json();
  console.log("LP:", data);
  poll();
}
poll();
```

---

# 3. C++ Client

```cpp
#include <vix/websocket/client.hpp>
#include <iostream>

int main() {
    vix::websocket::Client client("ws://localhost:9090/");

    client.on_open([]{ std::cout << "Connected!"; });

    client.on_message([](const std::string& m){
        std::cout << "Got: " << m << std::endl;
    });

    client.connect_blocking();
}
```

---

## Sending JSON from C++

```cpp
client.send_json(JsonMessage::serialize(
    "chat.message",
    {"user", "Bob", "room", "africa", "text", "Hi!"}
));
```

---

# 4. Reconnect Strategy (Coming Soon)

- WS -> LP fallback  
- Auto reconnect  
- Offline message queue  
