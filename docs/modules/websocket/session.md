# Session

This page explains `vix::websocket::Session`.

Use it when you want to understand how one WebSocket client connection is handled, how messages are sent, how frames are read, how the handshake works, how heartbeat is managed, and how sessions close safely.

## Header

```cpp
#include <vix/websocket/session.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What Session provides

`vix::websocket::Session` represents one connected WebSocket client.

It owns the lifecycle of one connection.

A session handles:

- HTTP Upgrade handshake
- WebSocket frame parsing
- text frame sending
- binary frame sending
- read loop
- write queue
- close handling
- ping/pong handling
- heartbeat lifecycle
- idle timeout behavior
- error dispatch
- immediate shutdown during server stop

Most applications do not create sessions manually.

They receive a session from server callbacks:

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("echo: " + message);
  });
```

## Basic model

A session sits between the low-level TCP stream and user callbacks.

```txt
TCP stream
  -> Session
  -> WebSocket handshake
  -> frame read loop
  -> Router
  -> Server callback
  -> user code
```

For one connected client:

```txt
client connects
  -> server accepts TCP stream
  -> session is created
  -> session performs HTTP Upgrade
  -> session starts read loop
  -> messages are dispatched
  -> session closes
```

## Session in the server flow

The complete server flow looks like this:

```txt
LowLevelServer
  -> accepts TCP client
  -> creates Session
  -> Session::run()
  -> Session::do_accept()
  -> Session::do_read_loop()
  -> Router callbacks
```

Application code usually sees only:

```cpp
vix::websocket::Session &session
```

inside callbacks.

## Constructor

The session constructor receives:

```cpp
Session(
    std::unique_ptr<tcp_stream> stream,
    const Config &cfg,
    std::shared_ptr<Router> router,
    std::shared_ptr<vix::executor::RuntimeExecutor> executor,
    std::shared_ptr<io_context> ioc);
```

Arguments:

| Argument   | Purpose                                                    |
| ---------- | ---------------------------------------------------------- |
| `stream`   | Accepted native TCP stream.                                |
| `cfg`      | WebSocket runtime configuration.                           |
| `router`   | Dispatches open, close, error, and message events.         |
| `executor` | Runtime executor used by the WebSocket stack.              |
| `ioc`      | Async I/O context used for reads, writes, and posted work. |

Most users do not call this constructor directly.

The low-level server creates sessions internally.

## run

`run()` starts the session lifecycle.

```cpp
task<void> run();
```

The flow is:

```txt
run
  -> do_accept
  -> maybe_start_heartbeat
  -> do_read_loop
  -> close on error
```

Conceptually:

```txt
Session::run()
  -> perform WebSocket handshake
  -> mark session open
  -> notify router open
  -> start heartbeat if configured
  -> read frames until closed
  -> notify router close
```

## HTTP Upgrade handshake

Before WebSocket messages can be exchanged, the client must send an HTTP Upgrade request.

The session validates the WebSocket handshake request.

The handshake checks headers such as:

```txt
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: ...
Sec-WebSocket-Version: 13
```

Then the server responds with:

```txt
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: ...
```

After this, the connection becomes a WebSocket connection.

## Open state

Use `is_open()` to check whether the session is currently open.

```cpp
if (session.is_open())
{
  session.send_text("hello");
}
```

The function returns:

```cpp
bool
```

Example:

```cpp
ws.on_open([](vix::websocket::Session &session)
{
  if (session.is_open())
  {
    session.send_text("welcome");
  }
});
```

## Send text

Use `send_text(...)` to send a text frame to this client.

```cpp
session.send_text("hello");
```

Inside a message handler:

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("received: " + message);
  });
```

This sends the message only to the current session.

## Send binary

Use `send_binary(...)` to send binary data.

```cpp
std::array<unsigned char, 4> data{1, 2, 3, 4};

session.send_binary(data.data(), data.size());
```

Signature:

```cpp
void send_binary(const void *data, std::size_t size);
```

Use binary messages for:

- compact protocol messages
- binary events
- small binary payloads
- application-specific frames

For large files, prefer HTTP upload or a dedicated storage flow.

## Close a session

Use `close(...)` to close the WebSocket session.

```cpp
session.close();
```

You can pass a reason:

```cpp
session.close("normal shutdown");
```

Signature:

```cpp
void close(std::string reason = {});
```

Use it when:

- the user logs out
- the protocol is violated
- the application wants to disconnect a client
- the server is shutting down
- a session is no longer authorized

## Immediate shutdown

Use `shutdown_now()` for server shutdown paths.

```cpp
session.shutdown_now();
```

This forces immediate transport shutdown.

It is intended for shutdown logic where the server must not depend on async close work being scheduled later.

Normal application code should prefer:

```cpp
session.close();
```

## Emit error

Use `emit_error(...)` to dispatch an error to the router error handler.

```cpp
session.emit_error("invalid message");
```

The server can receive this through:

```cpp
ws.on_error(
  [](vix::websocket::Session &session, const std::string &error)
  {
    (void)session;

    vix::print("error:", error);
  });
```

## Read loop

After the handshake, the session starts its read loop.

```txt
do_read_loop
  -> read_frame
  -> handle opcode
  -> dispatch message
  -> repeat
```

The read loop handles frame types such as:

```txt
text
binary
close
ping
pong
```

Text frames are dispatched to the router message handler.

Close frames begin connection shutdown.

Ping and pong frames are used for connection health.

## Frame reading

The session reads WebSocket frames from the TCP stream.

The frame model includes:

```txt
FIN bit
opcode
mask flag
mask key
payload length
payload bytes
```

Client-to-server frames are expected to be masked.

The session unpacks the frame, applies the mask when needed, and dispatches the payload.

## Text messages

A text frame becomes a string message.

```txt
WebSocket text frame
  -> Session
  -> Router::handle_message
  -> Server::on_message
```

Application handler:

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text(message);
  });
```

## Typed messages

Typed messages are parsed at the server layer after raw message dispatch.

The session itself reads the text frame.

Then the server can parse it as:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

Flow:

```txt
Session reads text frame
  -> Router dispatches raw message
  -> Server receives raw text
  -> Server parses JsonMessage
  -> Server calls on_typed_message
```

## Write queue

`send_text(...)` and `send_binary(...)` do not write directly from the caller stack.

They enqueue work into the session write queue.

Conceptually:

```txt
send_text
  -> post to io_context
  -> enqueue pending message
  -> flush write loop
  -> async write frame
```

This keeps writes ordered and avoids concurrent writes to the same stream.

## Write loop

The write loop drains pending messages one by one.

```txt
write queue
  -> take first message
  -> build WebSocket frame
  -> async write
  -> repeat until queue is empty
```

This means messages sent from the same session are written sequentially.

## Heartbeat

The session can start a heartbeat loop.

Heartbeat is controlled by:

```dotenv
WEBSOCKET_PING_INTERVAL=30
```

If the ping interval is greater than zero, the session can periodically send ping frames.

Use this to detect dead connections earlier.

## Ping and pong

WebSocket ping and pong frames are used to keep the connection alive and detect broken clients.

When automatic ping/pong is enabled:

```dotenv
WEBSOCKET_AUTO_PING_PONG=true
```

The session can handle ping/pong protocol behavior automatically.

For most applications, keep this enabled.

## Idle timeout

The session uses the configured idle timeout to close inactive connections.

```dotenv
WEBSOCKET_IDLE_TIMEOUT=60
```

Use `0` to disable idle timeout:

```dotenv
WEBSOCKET_IDLE_TIMEOUT=0
```

Idle timeout is useful for:

- cleaning dead connections
- reducing resource leaks
- avoiding abandoned sessions
- protecting public servers

## Maximum message size

The session uses the configured maximum message size to protect the server.

```dotenv
WEBSOCKET_MAX_MESSAGE_SIZE=65536
```

If a client sends a message larger than the limit, the session can reject it or close the connection.

Keep this value reasonable.

WebSocket is designed for realtime messages, not large file transfer.

## Close notification

The session notifies close handlers only once.

This prevents duplicate close events during error paths or shutdown paths.

Conceptually:

```txt
close happens
  -> notify router close once
  -> close stream
```

This keeps cleanup logic predictable.

## Error handling

Errors can happen during:

- handshake
- frame parsing
- async read
- async write
- ping/pong handling
- close handling
- transport shutdown

Register a server error handler:

```cpp
ws.on_error(
  [](vix::websocket::Session &session, const std::string &error)
  {
    (void)session;

    vix::print("WebSocket error:", error);
  });
```

## Example: welcome message

```cpp
ws.on_open([](vix::websocket::Session &session)
{
  session.send_text("welcome to Vix WebSocket");
});
```

## Example: echo server

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("echo: " + message);
  });
```

## Example: close on command

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    if (message == "close")
    {
      session.close("client requested close");
      return;
    }

    session.send_text("received: " + message);
  });
```

## Example: simple validation

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    if (message.empty())
    {
      session.emit_error("empty message");
      session.close("invalid message");
      return;
    }

    session.send_text("ok");
  });
```

## Example: binary response

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    (void)message;

    const unsigned char bytes[] = {1, 2, 3, 4};

    session.send_binary(bytes, sizeof(bytes));
  });
```

## Session and rooms

Rooms are managed by the server, not directly by the session.

To join a room, use the server API with the session shared pointer.

```cpp
ws.on_open([&ws](vix::websocket::Session &session)
{
  ws.join_room(session.shared_from_this(), "general");
});
```

Then the server can broadcast to the room:

```cpp
ws.broadcast_text_to_room("general", "hello room");
```

## Session and broadcasting

A session sends to one client:

```cpp
session.send_text("private message");
```

The server sends to many clients:

```cpp
ws.broadcast_text("global message");
```

The server sends to one room:

```cpp
ws.broadcast_text_to_room("general", "room message");
```

Use the right level:

| Need                | Use                              |
| ------------------- | -------------------------------- |
| Reply to one client | `session.send_text(...)`         |
| Send to all clients | `ws.broadcast_text(...)`         |
| Send to one room    | `ws.broadcast_text_to_room(...)` |

## Session lifecycle

A normal session lifecycle is:

```txt
created
  -> run
  -> accept handshake
  -> open
  -> read messages
  -> write messages
  -> close
  -> notify close
  -> cleanup
```

## Shutdown lifecycle

During server shutdown:

```txt
Server::stop_async
  -> collect live sessions
  -> session.shutdown_now()
  -> LowLevelServer::stop_async()
```

Then final shutdown can join threads:

```txt
Server::stop
  -> stop_async
  -> join low-level server threads
```

This separation keeps shutdown safe when called from different control paths.

## Best practices

Use `send_text(...)` for normal string responses.

Use `send_binary(...)` only for small binary protocol messages.

Use `close(...)` for normal application-level disconnection.

Use `shutdown_now()` only for server shutdown paths.

Use `on_error(...)` to observe session problems.

Keep message payloads small.

Use rooms and server broadcasting for multi-client delivery.

Avoid blocking inside callbacks.

## Common mistakes

### Sending after close

Avoid sending if the session is closed.

```cpp
if (session.is_open())
{
  session.send_text("hello");
}
```

### Using WebSocket for huge payloads

Avoid sending large files through WebSocket.

Use HTTP upload for large data.

### Doing heavy work in callbacks

Do not block session callbacks with expensive work.

Move heavy work to the runtime or another service layer.

### Calling shutdown_now manually in normal app logic

Prefer:

```cpp
session.close();
```

Reserve this for shutdown paths:

```cpp
session.shutdown_now();
```

## Next steps

Continue with:

- [Router](./router.md)
- [Messages](./messages.md)
- [Server](./server.md)
- [Rooms and Broadcasting](./rooms-and-broadcasting.md)
- [Shutdown](./shutdown.md)
