# Router

This page explains `vix::websocket::Router`.

Use it when you want to understand how WebSocket events are dispatched from sessions to application callbacks.

## Header

```cpp
#include <vix/websocket/router.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What Router provides

`vix::websocket::Router` is a lightweight event router for WebSocket sessions.

It dispatches four WebSocket events:

- open
- close
- error
- message

The router sits between the low-level session layer and the high-level server callbacks.

Most applications do not use `Router` directly.

They usually use:

```cpp
vix::websocket::Server ws{config, executor};

ws.on_open(...);
ws.on_close(...);
ws.on_error(...);
ws.on_message(...);
```

Internally, the server connects those callbacks to the router.

## Basic model

The event flow is:

```txt
Session
  -> Router
  -> Server callback
  -> user code
```

For example:

```txt
client sends text frame
  -> Session reads frame
  -> Router::handle_message(...)
  -> Server::on_message(...)
  -> application callback
```

## Router events

The router supports these handlers:

```cpp
router.on_open(...);
router.on_close(...);
router.on_error(...);
router.on_message(...);
```

And these dispatch functions:

```cpp
router.handle_open(session);
router.handle_close(session);
router.handle_error(session, error);
router.handle_message(session, payload);
```

## Handler types

The router defines these handler types:

```cpp
using OpenHandler =
    std::function<void(Session &)>;

using CloseHandler =
    std::function<void(Session &)>;

using ErrorHandler =
    std::function<void(Session &, const std::string &)>;

using MessageHandler =
    std::function<void(Session &, std::string)>;
```

## Open handler

Use `on_open(...)` to register a callback for new WebSocket sessions.

```cpp
vix::websocket::Router router;

router.on_open([](vix::websocket::Session &session)
{
  (void)session;

  vix::print("session opened");
});
```

The open handler is called when the session becomes active after the WebSocket handshake.

## Close handler

Use `on_close(...)` to register a callback for closed sessions.

```cpp
router.on_close([](vix::websocket::Session &session)
{
  (void)session;

  vix::print("session closed");
});
```

The close handler is useful for:

- cleanup
- logging
- presence updates
- removing session state
- notifying other clients

## Error handler

Use `on_error(...)` to register a callback for session errors.

```cpp
router.on_error(
  [](vix::websocket::Session &session, const std::string &error)
  {
    (void)session;

    vix::print("session error:", error);
  });
```

The error handler is useful for:

- debugging handshake failures
- detecting invalid frames
- logging connection resets
- diagnosing read/write failures

## Message handler

Use `on_message(...)` to register a callback for incoming text messages.

```cpp
router.on_message(
  [](vix::websocket::Session &session, std::string payload)
  {
    session.send_text("received: " + payload);
  });
```

The message payload is moved into the handler.

Use this for raw text messages.

Typed JSON parsing happens at the server layer.

## Dispatch open

`handle_open(...)` dispatches an open event.

```cpp
router.handle_open(session);
```

If no open handler is registered, nothing happens.

This makes dispatch safe even when the user did not configure every callback.

## Dispatch close

`handle_close(...)` dispatches a close event.

```cpp
router.handle_close(session);
```

If no close handler is registered, nothing happens.

## Dispatch error

`handle_error(...)` dispatches an error event.

```cpp
router.handle_error(session, "invalid frame");
```

If no error handler is registered, nothing happens.

## Dispatch message

`handle_message(...)` dispatches a text message.

```cpp
router.handle_message(session, "hello");
```

If no message handler is registered, nothing happens.

## Check registered handlers

The router exposes helper methods to check whether a handler exists.

```cpp
router.has_open_handler();
router.has_close_handler();
router.has_error_handler();
router.has_message_handler();
```

Each function returns:

```cpp
bool
```

Example:

```cpp
if (router.has_message_handler())
{
  vix::print("message handler registered");
}
```

## Router inside Server

Most applications use `Server`, not `Router`.

The server creates a router internally.

Conceptually:

```txt
Server constructor
  -> create Router
  -> create LowLevelServer
  -> register router callbacks
```

The server then maps router events to user callbacks.

```txt
Router::on_open
  -> register session
  -> call user on_open

Router::on_close
  -> unregister session
  -> remove session from rooms
  -> call user on_close

Router::on_error
  -> call user on_error

Router::on_message
  -> call user on_message
  -> parse typed JsonMessage
  -> forward to long-polling bridge
  -> call user on_typed_message
```

This keeps the router simple while allowing `Server` to provide higher-level features.

## Router vs Server callbacks

Use `Server` callbacks in normal applications.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("echo: " + message);
  });
```

Use `Router` directly only when you are working on lower-level WebSocket internals.

| Need                          | Use                             |
| ----------------------------- | ------------------------------- |
| Build normal realtime app     | `vix::websocket::Server`        |
| Receive raw messages          | `Server::on_message(...)`       |
| Receive typed JSON messages   | `Server::on_typed_message(...)` |
| Work on low-level dispatch    | `vix::websocket::Router`        |
| Connect custom session engine | `vix::websocket::Router`        |

## Router and Session

A session calls the router when lifecycle events happen.

```txt
Session opens
  -> router.handle_open(session)

Session receives text
  -> router.handle_message(session, text)

Session reports error
  -> router.handle_error(session, error)

Session closes
  -> router.handle_close(session)
```

The router does not own the session.

It only receives a reference and dispatches it to callbacks.

## Router and typed messages

The router only knows about raw messages.

It does not parse typed JSON messages.

Typed message parsing is handled by `Server`.

The flow is:

```txt
Session
  -> Router raw message
  -> Server raw handler
  -> JsonMessage::parse(...)
  -> Server typed handler
```

This separation keeps the router small and focused.

## Minimal direct router example

This is a low-level example.

Most applications should use `Server`.

```cpp
#include <vix/print.hpp>
#include <vix/websocket/router.hpp>

int main()
{
  vix::websocket::Router router;

  router.on_open([](vix::websocket::Session &session)
  {
    (void)session;

    vix::print("open");
  });

  router.on_error(
    [](vix::websocket::Session &session, const std::string &error)
    {
      (void)session;

      vix::print("error:", error);
    });

  return 0;
}
```

## Recommended server-level usage

For normal applications, write this instead:

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
      session.send_text("echo: " + message);
    });

  ws.on_close([](vix::websocket::Session &session)
  {
    (void)session;

    vix::print("client disconnected");
  });

  ws.start();

  return 0;
}
```

## Design goal

The router is intentionally small.

It does not manage:

- rooms
- broadcasts
- message persistence
- long-polling
- metrics
- typed message routing
- OpenAPI docs

Those features belong to the higher-level server and integration layers.

The router only dispatches events.

## Best practices

Use `Server` for application code.

Use `Router` for low-level integration code.

Keep router callbacks small.

Do not block inside router callbacks.

Use `on_error(...)` when debugging connection behavior.

Use typed messages through `Server::on_typed_message(...)`, not through `Router`.

## Common mistakes

### Using Router when Server is enough

For most apps, do not create a router manually.

Prefer:

```cpp
vix::websocket::Server ws{config, executor};
```

### Expecting Router to parse JSON

The router receives raw text only.

Use:

```cpp
ws.on_typed_message(...);
```

for typed JSON messages.

### Putting room logic inside Router

Rooms are a server-level feature.

Use:

```cpp
ws.join_room(...);
ws.broadcast_text_to_room(...);
```

### Blocking inside callbacks

Do not perform long blocking work directly in router callbacks.

Move heavy work to another runtime task or service layer.

## Next steps

Continue with:

- [Messages](./messages.md)
- [Server](./server.md)
- [Session](./session.md)
- [Rooms and Broadcasting](./rooms-and-broadcasting.md)
