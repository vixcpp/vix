# Server

This page explains how to use `vix::websocket::Server`.

Use it when you want to create a WebSocket server, receive messages, send responses, manage sessions, broadcast messages, use rooms, attach long-polling, and control shutdown.

## Header

```cpp
#include <vix/websocket/server.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What Server provides

`vix::websocket::Server` is the high-level WebSocket server API.

It wraps the low-level WebSocket engine and provides an application-friendly interface for realtime communication.

It provides:

- WebSocket server startup
- WebSocket server shutdown
- open callbacks
- close callbacks
- error callbacks
- raw message callbacks
- typed JSON message callbacks
- active session tracking
- rooms
- global broadcasting
- room broadcasting
- long-polling bridge integration
- access to the internal low-level engine

Most applications should use `Server` instead of `LowLevelServer`.

## Basic model

The server sits above the low-level engine and router.

```txt
Server
  -> LowLevelServer
  -> Router
  -> Session
  -> user callbacks
```

The flow is:

```txt
client connects
  -> LowLevelServer accepts TCP stream
  -> Session performs WebSocket handshake
  -> Router dispatches open/message/close/error
  -> Server invokes user callbacks
```

## Create a server

A server needs:

- a Vix configuration object
- a shared runtime executor

```cpp
#include <memory>

#include <vix/config/Config.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config config{".env"};

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{config, executor};

  ws.start();

  return 0;
}
```

## Constructor

The main constructor is:

```cpp
vix::websocket::Server::Server(
    vix::config::Config &cfg,
    std::shared_ptr<vix::executor::RuntimeExecutor> executor);
```

The server stores the config, creates a router, creates the low-level engine, and uses the shared executor.

```cpp
vix::websocket::Server ws{config, executor};
```

The executor must be valid.

If the executor is null, the constructor throws an exception.

## Unique executor constructor

The server can also receive an owning executor:

```cpp
auto executor =
    std::make_unique<vix::executor::RuntimeExecutor>(4);

vix::websocket::Server ws{config, std::move(executor)};
```

Internally, it is converted into a shared runtime executor.

## Start the server

Use `start()` to start the WebSocket engine.

```cpp
ws.start();
```

This starts the low-level server.

The low-level server creates the listener, starts I/O threads, and begins accepting WebSocket connections.

## Minimal echo server

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

    vix::print("client connected");
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

    vix::print("client disconnected");
  });

  ws.on_error(
    [](vix::websocket::Session &session, const std::string &error)
    {
      (void)session;

      vix::print("websocket error:", error);
    });

  ws.start();

  return 0;
}
```

## Event callbacks

The server exposes four main lifecycle callbacks:

```cpp
ws.on_open(...);
ws.on_close(...);
ws.on_error(...);
ws.on_message(...);
```

And one typed message callback:

```cpp
ws.on_typed_message(...);
```

## on_open

`on_open(...)` runs when a WebSocket session is opened.

```cpp
ws.on_open([](vix::websocket::Session &session)
{
  (void)session;

  vix::print("client connected");
});
```

Use it for:

- connection logs
- session initialization
- joining default rooms
- sending a welcome message
- incrementing custom counters

Example:

```cpp
ws.on_open([](vix::websocket::Session &session)
{
  session.send_text("welcome");
});
```

## on_close

`on_close(...)` runs when a session closes.

```cpp
ws.on_close([](vix::websocket::Session &session)
{
  (void)session;

  vix::print("client disconnected");
});
```

Use it for:

- cleanup
- logs
- presence updates
- removing application state
- notifying other clients

The server automatically unregisters the session and removes it from rooms during close handling.

## on_error

`on_error(...)` runs when a session reports an error.

```cpp
ws.on_error(
  [](vix::websocket::Session &session, const std::string &error)
  {
    (void)session;

    vix::print("error:", error);
  });
```

Use it for:

- logging
- diagnostics
- connection debugging
- protocol errors
- invalid frame handling

## on_message

`on_message(...)` runs when a raw text message is received.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("received: " + message);
  });
```

Use raw messages when:

- the protocol is simple
- the client sends plain text
- you want to parse the payload manually
- you want echo behavior
- you do not need typed event routing

## on_typed_message

`on_typed_message(...)` runs when an incoming text message can be parsed as a typed JSON message.

The expected JSON shape is:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

Register a typed handler:

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

Use typed messages when your WebSocket connection carries multiple application events.

Examples:

```txt
chat.message
notification.created
presence.update
room.join
room.leave
```

## Raw and typed handlers together

The server can use both raw and typed message handlers.

The raw handler receives the original text payload.

The typed handler receives parsed messages that match the typed JSON convention.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    (void)session;

    vix::print("raw:", message);
  });

ws.on_typed_message(
  [](vix::websocket::Session &session,
     const std::string &type,
     const vix::json::kvs &payload)
  {
    (void)session;
    (void)payload;

    vix::print("typed:", type);
  });
```

## Send a message to one client

Use `Session::send_text(...)`.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("you said: " + message);
  });
```

This sends a text frame only to the connected session.

## Active sessions

The server tracks active sessions internally.

Conceptually:

```txt
Server
  -> sessions_
    -> weak references to Session
```

The server registers a session when it opens.

The server unregisters it when it closes.

This lets the server broadcast messages and manage rooms without forcing application code to track every session manually.

## Rooms

Rooms group sessions under a string id.

Conceptually:

```txt
room: general
  -> session A
  -> session B

room: support
  -> session C
```

Rooms are useful for:

- chat rooms
- private channels
- user-specific updates
- tenant-specific events
- project dashboards
- multiplayer game rooms

## Join a room

Use the server room API to put a session into a room.

```cpp
ws.on_open([&ws](vix::websocket::Session &session)
{
  ws.join_room(session.shared_from_this(), "general");
});
```

After this, the session belongs to the `general` room.

## Leave a room

Use the room leave API to remove a session from a room.

```cpp
ws.leave_room(session.shared_from_this(), "general");
```

Use this when:

- a client leaves a chat room
- a user unsubscribes from a channel
- a dashboard switches context
- a game player leaves a match

## Broadcast to all sessions

Use global broadcast when every connected WebSocket client should receive a message.

```cpp
ws.broadcast_text("server update");
```

Use global broadcast for:

- system-wide notifications
- global status changes
- maintenance messages
- application-wide events

## Broadcast to a room

Use room broadcast when only clients in one room should receive a message.

```cpp
ws.broadcast_text_to_room("general", "hello room");
```

Use room broadcast for:

- chat rooms
- project updates
- user groups
- tenant isolation
- live dashboard channels

## Typed room example

```cpp
ws.on_typed_message(
  [&ws](vix::websocket::Session &session,
        const std::string &type,
        const vix::json::kvs &payload)
  {
    (void)payload;

    if (type == "room.join")
    {
      ws.join_room(session.shared_from_this(), "general");
      session.send_text("joined room general");
      return;
    }

    if (type == "chat.message")
    {
      ws.broadcast_text_to_room("general", "new chat message");
      return;
    }
  });
```

## Attach long-polling

The server can forward typed WebSocket messages to a long-polling bridge.

This lets HTTP polling clients receive messages from the same realtime system.

```cpp
vix::websocket::WebSocketMetrics metrics;

vix::websocket::LongPollingBridge bridge{
    &metrics,
    std::chrono::seconds{60},
    256};

ws.attach_long_polling_bridge(&bridge);
```

With a bridge attached:

```txt
WebSocket typed message
  -> Server parses JsonMessage
  -> LongPollingBridge receives message
  -> LongPollingManager buffers message
  -> HTTP polling clients can receive it
```

## Long-polling resolver

By default, `LongPollingBridge` resolves messages like this:

```txt
if message.room is not empty:
  session id = "room:" + message.room
else:
  session id = "broadcast"
```

This makes room-based fallback delivery possible.

## Stop asynchronously

Use `stop_async()` when shutdown must not block.

```cpp
ws.stop_async();
```

This function:

- collects live sessions
- requests immediate session shutdown
- asks the low-level engine to stop asynchronously
- does not join internal worker threads

This is useful inside shutdown callbacks.

## Stop and join

Use `stop()` when you want final blocking shutdown.

```cpp
ws.stop();
```

This function stops the WebSocket server and joins internal threads.

Use it from a safe control path, not from a callback that must remain non-blocking.

## Server lifecycle

A normal server lifecycle is:

```txt
create config
  -> create RuntimeExecutor
  -> create Server
  -> register callbacks
  -> start server
  -> accept clients
  -> stop server
  -> stop executor
```

Example:

```cpp
int main()
{
  vix::config::Config config{".env"};

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{config, executor};

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      session.send_text(message);
    });

  ws.start();

  ws.stop();
  executor->stop();

  return 0;
}
```

## Server with HTTP app

Most real applications use WebSocket together with HTTP.

Use `AttachedRuntime` for that.

```cpp
#include <memory>

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

In this model:

```txt
HTTP app
  -> runs on SERVER_PORT

WebSocket server
  -> runs on WEBSOCKET_PORT

AttachedRuntime
  -> coordinates shutdown
```

## Server configuration

The server uses the core config object.

Common `.env` values:

```dotenv
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

The server host and port are used by the low-level engine.

The other values are converted into `vix::websocket::Config` and passed to sessions.

## Error handling

Use `on_error(...)` to catch WebSocket session errors.

```cpp
ws.on_error(
  [](vix::websocket::Session &session, const std::string &error)
  {
    (void)session;

    vix::print("WebSocket error:", error);
  });
```

Common errors can come from:

- invalid handshake
- invalid frame
- connection reset
- oversized message
- read failure
- write failure
- closed connection

## Threading model

The server uses the low-level WebSocket engine.

The engine owns:

```txt
io_context
TCP listener
I/O worker threads
accept loop
sessions
```

The server uses `RuntimeExecutor` consistently across the WebSocket stack.

This keeps the runtime model aligned with the rest of Vix.

## Best practices

Register all callbacks before calling:

```cpp
ws.start();
```

Use typed messages for real application protocols.

Use rooms instead of global broadcast when only a subset of clients should receive an event.

Use `stop_async()` from callbacks.

Use `stop()` from safe shutdown paths.

Use `AttachedRuntime` when running HTTP and WebSocket together.

Use a message store when clients need replay or history.

Use metrics when the server is part of a production service.

## Common mistakes

### Null executor

This is invalid:

```cpp
std::shared_ptr<vix::executor::RuntimeExecutor> executor;

vix::websocket::Server ws{config, executor};
```

Create a valid executor:

```cpp
auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);
```

### Blocking inside shutdown callback

Do not call blocking shutdown from a callback that must remain non-blocking.

Prefer:

```cpp
ws.stop_async();
```

Then finalize later with:

```cpp
ws.stop();
```

### Broadcasting everything globally

Avoid sending every event to every client.

Prefer room-based delivery when possible:

```cpp
ws.broadcast_text_to_room("general", message);
```

### Sending huge payloads through WebSocket

WebSocket is best for realtime messages.

For large files, use HTTP upload or a dedicated storage flow.

## Next steps

Continue with:

- [Session](./session.md)
- [Router](./router.md)
- [Messages](./messages.md)
- [Rooms and Broadcasting](./rooms-and-broadcasting.md)
- [Attached runtime](./attached-runtime.md)
