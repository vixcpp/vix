# Client

This page explains `vix::websocket::Client`.

Use it when you want to connect to a WebSocket server from C++, send text messages, send typed JSON messages, receive messages, handle errors, use heartbeat, and enable auto-reconnect.

## Header

```cpp
#include <vix/websocket/client.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What Client provides

`vix::websocket::Client` is a native WebSocket client for Vix.

It provides:

- asynchronous DNS and TCP connection
- WebSocket HTTP Upgrade handshake
- text frame sending
- text frame receiving
- typed JSON message sending
- open callback
- message callback
- close callback
- error callback
- heartbeat ping
- optional auto-reconnect
- safe close behavior

It is independent of Boost.Beast and Boost.Asio.

## Basic model

The client flow is:

```txt
Client::create(...)
  -> configure callbacks
  -> connect()
  -> resolve host
  -> connect TCP
  -> perform WebSocket handshake
  -> emit open
  -> read messages
  -> emit message callbacks
```

For outgoing messages:

```txt
send_text(...)
  -> enqueue frame
  -> flush write queue
  -> async write
```

## Create a client

Use `Client::create(...)`.

```cpp
auto client =
    vix::websocket::Client::create("127.0.0.1", "9090", "/");
```

The arguments are:

| Argument | Purpose                         |
| -------- | ------------------------------- |
| `host`   | Hostname or IP address.         |
| `port`   | Service or port as a string.    |
| `target` | WebSocket path, default is `/`. |

Example URLs:

```txt
ws://127.0.0.1:9090/
ws://localhost:9090/
ws://example.com:9090/chat
```

## Minimal client

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
    vix::print("connected");

    client->send_text("hello from Vix client");
  });

  client->on_message([](const std::string &message)
  {
    vix::print("received:", message);
  });

  client->on_close([]()
  {
    vix::print("closed");
  });

  client->on_error([](const std::string &error)
  {
    vix::print("error:", error);
  });

  client->connect();

  return 0;
}
```

## Open callback

Use `on_open(...)` to run code after the WebSocket handshake succeeds.

```cpp
client->on_open([client]()
{
  client->send_text("hello");
});
```

Use this callback for:

- sending the first message
- authentication messages
- room join messages
- startup logs
- client initialization

## Message callback

Use `on_message(...)` to receive text frames.

```cpp
client->on_message([](const std::string &message)
{
  vix::print("server:", message);
});
```

The callback receives the text payload as a string.

## Close callback

Use `on_close(...)` to detect when the connection closes.

```cpp
client->on_close([]()
{
  vix::print("connection closed");
});
```

Use it for:

- cleanup
- reconnect diagnostics
- lifecycle logs
- UI state updates
- tests

## Error callback

Use `on_error(...)` to receive client errors.

```cpp
client->on_error([](const std::string &error)
{
  vix::print("client error:", error);
});
```

Errors can happen during:

- DNS resolution
- TCP connect
- WebSocket handshake
- read loop
- write loop
- ping
- close

## Connect

Use `connect()` to start the client.

```cpp
client->connect();
```

`connect()` initializes the runtime, starts the I/O thread, and begins the async connection flow.

Call `connect()` after registering callbacks.

```cpp
client->on_open(...);
client->on_message(...);
client->on_error(...);

client->connect();
```

## Send text

Use `send_text(...)` to send a raw text message.

```cpp
client->send_text("hello");
```

Example:

```cpp
client->on_open([client]()
{
  client->send_text("hello from client");
});
```

`send_text(...)` queues the outgoing frame and flushes writes sequentially.

## Send typed JSON messages

Use `send_json_message(...)` to send a typed JSON message.

```cpp
client->send_json_message(
    "chat.message",
    {
      {"text", "Hello from client"}
    });
```

This sends a message shaped like:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello from client"
  }
}
```

## Send typed JSON with kvs

You can pass a `vix::json::kvs` payload.

```cpp
vix::json::kvs payload{
  {"text", "Hello"},
  {"room", "general"}
};

client->send_json_message("chat.message", payload);
```

## Send typed JSON with initializer list

You can also pass inline tokens.

```cpp
client->send_json_message(
    "notification.created",
    {
      {"title", "New order"},
      {"order_id", "A-100"}
    });
```

## Full typed client example

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
    vix::print("connected");

    client->send_json_message(
        "chat.message",
        {
          {"text", "Hello from Vix client"}
        });
  });

  client->on_message([](const std::string &message)
  {
    vix::print("server:", message);
  });

  client->on_error([](const std::string &error)
  {
    vix::print("error:", error);
  });

  client->connect();

  return 0;
}
```

## Heartbeat

Use `enable_heartbeat(...)` to send periodic ping frames.

```cpp
client->enable_heartbeat(std::chrono::seconds{30});
```

Then connect:

```cpp
client->connect();
```

Full example:

```cpp
auto client =
    vix::websocket::Client::create("127.0.0.1", "9090", "/");

client->enable_heartbeat(std::chrono::seconds{30});

client->connect();
```

Use heartbeat when you want to:

- keep connections alive
- detect broken connections
- monitor connection health
- avoid silent dead sockets

To disable heartbeat, pass a non-positive interval:

```cpp
client->enable_heartbeat(std::chrono::seconds{0});
```

## Auto-reconnect

Use `enable_auto_reconnect(...)`.

```cpp
client->enable_auto_reconnect(true, std::chrono::seconds{3});
```

This tells the client to attempt reconnection after failures.

Example:

```cpp
auto client =
    vix::websocket::Client::create("127.0.0.1", "9090", "/");

client->enable_auto_reconnect(true, std::chrono::seconds{3});

client->connect();
```

Use auto-reconnect for:

- local agents
- background services
- dashboards
- development tools
- internal realtime clients

## Heartbeat with auto-reconnect

```cpp
auto client =
    vix::websocket::Client::create("127.0.0.1", "9090", "/");

client->enable_heartbeat(std::chrono::seconds{30});
client->enable_auto_reconnect(true, std::chrono::seconds{3});

client->on_open([client]()
{
  client->send_json_message(
      "client.ready",
      {
        {"name", "worker-1"}
      });
});

client->on_message([](const std::string &message)
{
  vix::print("message:", message);
});

client->on_error([](const std::string &error)
{
  vix::print("error:", error);
});

client->connect();
```

## Ping

Use `send_ping()` to send a ping frame manually.

```cpp
client->send_ping();
```

This is useful when:

- you want manual heartbeat behavior
- you are testing server ping/pong handling
- you want explicit connection health checks

Most applications should use `enable_heartbeat(...)`.

## Close

Use `close()` to close the connection and stop client threads.

```cpp
client->close();
```

`close()` is safe to call multiple times.

It:

- marks the client as closing
- disables reconnect
- stops heartbeat
- requests cancellation
- attempts to send a close frame
- closes the stream
- stops the I/O context
- joins the I/O thread when safe

## Close example

```cpp
client->on_message([client](const std::string &message)
{
  if (message == "bye")
  {
    client->close();
    return;
  }

  vix::print("message:", message);
});
```

## Threading model

The client runs its own I/O context and I/O thread.

Conceptually:

```txt
connect()
  -> init io_context
  -> start io thread
  -> spawn connect flow
```

The I/O thread runs the async runtime for the client.

Writes are queued and flushed sequentially.

## Write queue

Outgoing frames are stored in a write queue.

```txt
send_text
  -> build frame
  -> push to write queue
  -> trigger write flush
```

This prevents concurrent writes to the same TCP stream.

## Client frame masking

Client WebSocket frames are masked.

This is required by the WebSocket protocol for client-to-server frames.

When the client sends text, JSON, ping, or close frames, it builds masked frames.

## Typed client and server

A Vix client can talk to a Vix server using the typed JSON convention.

Server:

```cpp
ws.on_typed_message(
  [](vix::websocket::Session &session,
     const std::string &type,
     const vix::json::kvs &payload)
  {
    (void)payload;

    if (type == "chat.message")
    {
      session.send_text("message received");
    }
  });
```

Client:

```cpp
client->on_open([client]()
{
  client->send_json_message(
      "chat.message",
      {
        {"text", "Hello"}
      });
});
```

## Client as a test tool

The client is useful for integration tests.

Example:

```cpp
auto client =
    vix::websocket::Client::create("127.0.0.1", "9090", "/");

client->on_open([client]()
{
  client->send_text("ping");
});

client->on_message([](const std::string &message)
{
  if (message == "pong")
  {
    vix::print("test passed");
  }
});

client->connect();
```

## Client as an internal service

You can also use the client inside internal services.

Example use cases:

- local worker connects to a control server
- CLI connects to a realtime API
- agent streams status updates
- test runner connects to a Vix server
- monitoring service listens for events

## Browser client comparison

A browser client looks like this:

```js
const socket = new WebSocket("ws://localhost:9090");

socket.onopen = () => {
  socket.send(
    JSON.stringify({
      type: "chat.message",
      payload: {
        text: "Hello",
      },
    }),
  );
};

socket.onmessage = (event) => {
  console.log(event.data);
};
```

The Vix C++ client equivalent is:

```cpp
auto client =
    vix::websocket::Client::create("127.0.0.1", "9090", "/");

client->on_open([client]()
{
  client->send_json_message(
      "chat.message",
      {
        {"text", "Hello"}
      });
});

client->on_message([](const std::string &message)
{
  vix::print(message);
});

client->connect();
```

## Best practices

Register callbacks before calling `connect()`.

Use `send_json_message(...)` for structured application protocols.

Use `send_text(...)` for simple tests and debugging.

Enable heartbeat for long-lived clients.

Enable auto-reconnect for agents or background services.

Always register `on_error(...)` during development.

Call `close()` during shutdown.

Keep messages small.

## Common mistakes

### Calling connect before callbacks

Avoid:

```cpp
client->connect();

client->on_message(...);
```

Prefer:

```cpp
client->on_message(...);

client->connect();
```

### Sending before connection opens

Avoid sending immediately after `connect()`.

```cpp
client->connect();
client->send_text("hello");
```

Prefer sending inside `on_open(...)`.

```cpp
client->on_open([client]()
{
  client->send_text("hello");
});

client->connect();
```

### Forgetting error handling

Always add an error handler during development.

```cpp
client->on_error([](const std::string &error)
{
  vix::print("error:", error);
});
```

### Using WebSocket for large files

Do not send large files through WebSocket.

Use HTTP upload or another storage path.

### Enabling reconnect for intentional shutdown

`close()` disables auto-reconnect.

Use it when shutdown is intentional.

## Next steps

Continue with:

- [Messages](./messages.md)
- [Rooms and Broadcasting](./rooms-and-broadcasting.md)
- [Long polling](./long-polling.md)
- [Shutdown](./shutdown.md)
