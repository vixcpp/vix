# WebSocket API

This page documents the WebSocket server API in Vix.

It covers:

-   WebSocket server
-   Event hooks
-   Typed message protocol
-   Broadcast functions
-   Long polling bridge

All examples are minimal and placed inside `main()`.

------------------------------------------------------------------------

# Server

Header:

``` cpp
#include <vix/websocket.hpp>
```

Core type:

``` cpp
vix::websocket::Server
```

------------------------------------------------------------------------

## Constructor

``` cpp
Server ws(cfg, executor);
```

Parameters:

-   `cfg` → configuration object
-   `executor` → thread pool executor

------------------------------------------------------------------------

# Minimal Standalone Example

``` cpp
#include <vix/config/Config.hpp>
#include <vix/experimental/ThreadPoolExecutor.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config cfg{"config.json"};

  auto exec = vix::experimental::make_threadpool_executor(4, 8, 0);

  vix::websocket::Server ws(cfg, std::move(exec));

  ws.on_open([](auto& session) {
    (void)session;
  });

  ws.on_typed_message([](auto& session,
                         const std::string& type,
                         const vix::json::kvs& payload)
  {
    (void)session;
    (void)type;
    (void)payload;
  });

  ws.listen_blocking();

  return 0;
}
```

------------------------------------------------------------------------

# Event Hooks

## on_open

Called when a client connects.

``` cpp
ws.on_open([](auto& session) {
  // session is active
});
```

------------------------------------------------------------------------

## on_close

Called when a client disconnects.

``` cpp
ws.on_close([](auto& session) {
  // cleanup
});
```

------------------------------------------------------------------------

## on_typed_message

Called when a typed message is received.

Signature:

``` cpp
(session, type, payload)
```

-   `type` → string
-   `payload` → `vix::json::kvs`

------------------------------------------------------------------------

# Typed Message Protocol

Expected message shape:

``` json
{
  "type": "event.name",
  "payload": { ... }
}
```

Vix automatically parses and routes this to `on_typed_message`.

------------------------------------------------------------------------

# Broadcast

## Broadcast to all

``` cpp
ws.broadcast_json("chat.message", {
  "user", "Alice",
  "text", "Hello"
});
```

------------------------------------------------------------------------

## Broadcast to room

``` cpp
ws.broadcast_room_json("room1", "chat.message", payload);
```

------------------------------------------------------------------------

# Long Polling Bridge

Header:

``` cpp
#include <vix/websocket/LongPollingBridge.hpp>
```

Attach bridge:

``` cpp
ws.attach_long_polling_bridge(bridge);
```

Purpose:

-   Fallback when WebSocket is blocked
-   HTTP endpoints simulate WebSocket behavior

------------------------------------------------------------------------

# HTTP + WS Combined

Use helper:

``` cpp
vix::serve_http_and_ws([](auto& app, auto& ws) {
  // register HTTP routes
  // register WebSocket handlers
});
```

This creates a single runtime handling both protocols.

------------------------------------------------------------------------

# Notes

-   `listen_blocking()` blocks the thread.
-   WebSocket handlers should be short and non-blocking.
-   Use broadcast carefully in high-load scenarios.
-   Use room broadcasting for scalable chat systems.

The WebSocket API is minimal by design.

