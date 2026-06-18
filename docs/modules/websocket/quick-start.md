# Quick Start

This page shows how to start using the Vix WebSocket module quickly.

You will create a WebSocket server, listen for incoming messages, send responses, and understand the minimal runtime setup.

## Header

Use the WebSocket umbrella header:

```cpp
#include <vix/websocket.hpp>
```

This gives access to the main public APIs:

```cpp
vix::websocket::Server
vix::websocket::Session
vix::websocket::Client
vix::websocket::App
vix::websocket::AttachedRuntime
vix::websocket::WebSocketMetrics
```

## Minimal WebSocket server

A WebSocket server needs:

1. a Vix configuration
2. a runtime executor
3. a WebSocket server
4. message callbacks
5. server startup

```cpp
#include <memory>
#include <string>

#include <vix/config/Config.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/print.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config config{".env"};

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{config, executor};

  ws.on_open([](vix::websocket::Session &session)
  {
    (void)session;

    vix::print("WebSocket client connected");
  });

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      vix::print("received:", message);

      session.send_text("echo: " + message);
    });

  ws.on_close([](vix::websocket::Session &session)
  {
    (void)session;

    vix::print("WebSocket client disconnected");
  });

  ws.start();

  return 0;
}
```

## Configuration

Create a `.env` file:

```dotenv
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090

WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

By default, the WebSocket server uses:

```txt
host = 0.0.0.0
port = 9090
```

## Run the example

```bash
vix run main.cpp
```

The server listens for WebSocket clients on:

```txt
ws://localhost:9090
```

## Test from a browser

You can test the server from the browser console:

```js
const socket = new WebSocket("ws://localhost:9090");

socket.onopen = () => {
  console.log("connected");
  socket.send("hello from browser");
};

socket.onmessage = (event) => {
  console.log("message:", event.data);
};

socket.onclose = () => {
  console.log("closed");
};

socket.onerror = (error) => {
  console.error("error:", error);
};
```

Expected output:

```txt
message: echo: hello from browser
```

## Raw text messages

Use `on_message(...)` to receive raw text frames.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("received: " + message);
  });
```

Use `send_text(...)` to send a text frame to one client.

```cpp
session.send_text("hello");
```

## Typed JSON messages

The WebSocket module also supports a typed JSON convention.

A typed message has this shape:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

Register a typed message handler:

```cpp
ws.on_typed_message(
  [](vix::websocket::Session &session,
     const std::string &type,
     const vix::json::kvs &payload)
  {
    (void)payload;

    if (type == "chat.message")
    {
      session.send_text("chat message received");
    }
  });
```

From JavaScript:

```js
socket.send(
  JSON.stringify({
    type: "chat.message",
    payload: {
      text: "Hello",
    },
  }),
);
```

## Send JSON from the C++ client

The native client can send typed JSON messages.

```cpp
auto client = vix::websocket::Client::create(
    "127.0.0.1",
    "9090",
    "/");

client->on_open([client]()
{
  client->send_json_message(
      "chat.message",
      {
        {"text", "Hello from Vix client"}
      });
});

client->on_message([](const std::string &message)
{
  vix::print("received:", message);
});

client->connect();
```

## Minimal WebSocket client

```cpp
#include <memory>
#include <string>

#include <vix/print.hpp>
#include <vix/websocket.hpp>

int main()
{
  auto client =
      vix::websocket::Client::create("127.0.0.1", "9090", "/");

  client->on_open([client]()
  {
    vix::print("client connected");

    client->send_text("hello from client");
  });

  client->on_message([](const std::string &message)
  {
    vix::print("server:", message);
  });

  client->on_close([]()
  {
    vix::print("client closed");
  });

  client->on_error([](const std::string &error)
  {
    vix::print("client error:", error);
  });

  client->connect();

  return 0;
}
```

## Connect WebSocket with HTTP

Most real applications need HTTP and WebSocket together.

Use `vix::websocket::AttachedRuntime` to attach a WebSocket server to a `vix::App`.

```cpp
#include <memory>
#include <string>

#include <vix.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::App app;

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{app.config(), executor};

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      session.send_text("echo: " + message);
    });

  vix::websocket::AttachedRuntime runtime{app, ws, executor};

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("HTTP and WebSocket are running");
  });

  app.run(8080);

  return 0;
}
```

HTTP runs on:

```txt
http://localhost:8080
```

WebSocket runs on:

```txt
ws://localhost:9090
```

## Long-polling fallback

The module includes a long-polling bridge for environments where WebSocket is unavailable.

The basic idea is:

```txt
WebSocket message
  -> LongPollingBridge
  -> LongPollingManager
  -> HTTP client polls messages
```

Common fallback endpoints are:

```txt
GET  /ws/poll
POST /ws/send
```

Long-polling is useful when:

- WebSocket is blocked by a proxy
- the client environment does not support WebSocket
- you need an HTTP-only fallback
- you want progressive realtime behavior

## Metrics

Create a metrics object:

```cpp
vix::websocket::WebSocketMetrics metrics;
```

Render Prometheus text:

```cpp
std::string text = metrics.render_prometheus();
```

Run a minimal metrics exporter:

```cpp
vix::websocket::run_metrics_http_exporter(metrics, "0.0.0.0", 9100);
```

Then metrics are available at:

```txt
http://localhost:9100/metrics
```

## Message persistence

Use `IMessageStore` when you want to abstract message history.

Use `SqliteMessageStore` when you want SQLite-backed persistence.

```cpp
vix::websocket::SqliteMessageStore store{"messages.db"};
```

Append a typed message:

```cpp
vix::websocket::JsonMessage message;
message.type = "chat.message";
message.room = "general";
message.payload = {
  {"text", "Hello"}
};

store.append(message);
```

List messages by room:

```cpp
auto messages = store.list_by_room("general", 50);
```

## Shutdown

Use `stop_async()` when shutdown must be non-blocking.

```cpp
ws.stop_async();
```

Use `stop()` when you want to stop and join the server completely.

```cpp
ws.stop();
```

When HTTP and WebSocket are attached together, `AttachedRuntime` coordinates shutdown safely.

The important rule is:

```txt
Do not block inside the HTTP shutdown callback.
Request async WebSocket stop first.
Finalize blocking shutdown later.
```

## What to read next

Continue with:

- [Concepts](./concepts.md)
- [Configuration](./configuration.md)
- [Server](./server.md)
- [Messages](./messages.md)
- [Attached runtime](./attached-runtime.md)
