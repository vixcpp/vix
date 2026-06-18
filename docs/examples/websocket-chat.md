# WebSocket Chat

This example shows how to build a small realtime chat server with the Vix WebSocket module.

It demonstrates:

```txt id="x6em4t"
WebSocket server startup
client connection callbacks
raw text messages
typed JSON messages
rooms
room broadcasts
HTTP health route
shared RuntimeExecutor
configuration from .env
```

Use this example when you want to build realtime features such as:

```txt id="fek4ip"
chat
notifications
live dashboards
collaboration
presence
room-based events
```

## What this example builds

The app runs:

```txt id="e9u21s"
HTTP server
  http://127.0.0.1:8080

WebSocket server
  ws://127.0.0.1:9090
```

The HTTP server exposes:

```txt id="dm6plx"
GET /
GET /api/health
```

The WebSocket server handles:

```txt id="ijwhm8"
client connected
client disconnected
raw text message
typed JSON chat.message
typed JSON room.join
typed JSON room.leave
```

## Project structure

Create:

```txt id="ur61wl"
websocket_chat_demo/
├── .env
├── websocket_chat.cpp
└── public/
    └── index.html
```

## .env

Create:

```txt id="s07ean"
.env
```

Add:

```dotenv id="ndzeaa"
APP_NAME=websocket-chat-demo
APP_ENV=development

SERVER_HOST=0.0.0.0
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=5000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=20
SERVER_BENCH_MODE=false

WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true

PUBLIC_PATH=public
PUBLIC_MOUNT=/
PUBLIC_INDEX=index.html
PUBLIC_CACHE_CONTROL=no-cache
PUBLIC_SPA_FALLBACK=false
```

The important WebSocket values are:

```txt id="sjlbtb"
WEBSOCKET_HOST
WEBSOCKET_PORT
WEBSOCKET_MAX_MESSAGE_SIZE
WEBSOCKET_IDLE_TIMEOUT
WEBSOCKET_ENABLE_DEFLATE
WEBSOCKET_PING_INTERVAL
WEBSOCKET_AUTO_PING_PONG
```

## public/index.html

Create:

```txt id="u7ltpu"
public/index.html
```

Add:

```html id="qfjq32"
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Vix WebSocket Chat</title>
    <style>
      body {
        margin: 0;
        font-family: system-ui, sans-serif;
        background: #f6f7f9;
        color: #111827;
      }

      .page {
        max-width: 760px;
        margin: 60px auto;
        padding: 24px;
        background: white;
        border: 1px solid #e5e7eb;
        border-radius: 16px;
      }

      .row {
        display: flex;
        gap: 8px;
        margin-top: 16px;
      }

      input {
        flex: 1;
        padding: 10px 12px;
        border: 1px solid #d1d5db;
        border-radius: 10px;
      }

      button {
        padding: 10px 14px;
        border: 0;
        border-radius: 10px;
        cursor: pointer;
      }

      pre {
        min-height: 260px;
        margin-top: 20px;
        padding: 16px;
        background: #111827;
        color: #f9fafb;
        border-radius: 12px;
        overflow: auto;
        white-space: pre-wrap;
      }
    </style>
  </head>
  <body>
    <main class="page">
      <h1>Vix WebSocket Chat</h1>
      <p>Open this page in two browser tabs and send messages.</p>

      <div class="row">
        <input id="room" value="general" placeholder="room" />
        <button id="join">Join room</button>
      </div>

      <div class="row">
        <input id="message" placeholder="message" />
        <button id="send">Send</button>
      </div>

      <pre id="log"></pre>
    </main>

    <script>
      const log = document.querySelector("#log");
      const roomInput = document.querySelector("#room");
      const messageInput = document.querySelector("#message");
      const joinButton = document.querySelector("#join");
      const sendButton = document.querySelector("#send");

      let currentRoom = "general";

      function write(line) {
        log.textContent += line + "\n";
        log.scrollTop = log.scrollHeight;
      }

      const socket = new WebSocket("ws://127.0.0.1:9090/");

      socket.addEventListener("open", () => {
        write("connected");

        socket.send(
          JSON.stringify({
            type: "room.join",
            payload: {
              room: currentRoom,
            },
          }),
        );
      });

      socket.addEventListener("message", (event) => {
        write("server: " + event.data);
      });

      socket.addEventListener("close", () => {
        write("closed");
      });

      socket.addEventListener("error", () => {
        write("error");
      });

      joinButton.addEventListener("click", () => {
        currentRoom = roomInput.value || "general";

        socket.send(
          JSON.stringify({
            type: "room.join",
            payload: {
              room: currentRoom,
            },
          }),
        );

        write("joined room: " + currentRoom);
      });

      sendButton.addEventListener("click", () => {
        const text = messageInput.value;

        if (!text) {
          return;
        }

        socket.send(
          JSON.stringify({
            type: "chat.message",
            payload: {
              room: currentRoom,
              text: text,
            },
          }),
        );

        messageInput.value = "";
      });
    </script>
  </body>
</html>
```

This browser page connects to:

```txt id="mfhvzp"
ws://127.0.0.1:9090/
```

and sends typed JSON messages.

## websocket_chat.cpp

Create:

```txt id="omksuo"
websocket_chat.cpp
```

Add:

```cpp id="tc9ksf"
#include <memory>
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>
#include <vix/websocket.hpp>
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix;

static std::string payload_string(
  const vix::json::kvs &payload,
  const std::string &key,
  const std::string &fallback)
{
  auto it = payload.find(key);

  if (it == payload.end())
    return fallback;

  return it->second;
}

static void register_http(vix::App &app)
{
  app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "no-cache",
    true,
    false
  );

  app.get("/api/health", [](vix::Request &, vix::Response &res)
  {
    res.json({
      "ok", true,
      "service", "websocket-chat"
    });
  });
}

static void register_websocket_handlers(vix::websocket::Server &ws)
{
  ws.on_open([](vix::websocket::Session &session)
  {
    session.send_text("welcome to Vix WebSocket chat");
    vix::print("client connected");
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

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      vix::print("raw message:", message);

      if (message == "ping")
      {
        session.send_text("pong");
      }
    });

  ws.on_typed_message(
    [&ws](vix::websocket::Session &session,
          const std::string &type,
          const vix::json::kvs &payload)
    {
      if (type == "room.join")
      {
        const std::string room =
          payload_string(payload, "room", "general");

        ws.join_room(session.shared_from_this(), room);

        session.send_text("joined room: " + room);
        return;
      }

      if (type == "room.leave")
      {
        const std::string room =
          payload_string(payload, "room", "general");

        ws.leave_room(session.shared_from_this(), room);

        session.send_text("left room: " + room);
        return;
      }

      if (type == "chat.message")
      {
        const std::string room =
          payload_string(payload, "room", "general");

        const std::string text =
          payload_string(payload, "text", "");

        if (text.empty())
        {
          session.send_text("message rejected: empty text");
          return;
        }

        ws.broadcast_text_to_room(
          room,
          "room " + room + ": " + text
        );

        return;
      }

      session.send_text("unknown message type: " + type);
    });
}

int main()
{
  vix::config::Config config{".env"};
  vix::App app;

  auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{config, executor};

  register_http(app);
  register_websocket_handlers(ws);

  vix::websocket::AttachedRuntime runtime{
    app,
    ws,
    executor
  };

  app.run(config);

  return 0;
}
```

## Run it

From the project directory:

```bash id="pc4ddk"
vix run websocket_chat.cpp
```

The app starts:

```txt id="crlybe"
HTTP
  http://127.0.0.1:8080

WebSocket
  ws://127.0.0.1:9090
```

Open the browser:

```txt id="qf8skp"
http://127.0.0.1:8080/
```

Open the page in two browser tabs.

Join the same room and send a message.

Both tabs should receive the room broadcast.

## Test HTTP health

```bash id="lndkgc"
curl -i http://127.0.0.1:8080/api/health
```

Expected body shape:

```json id="d86dws"
{
  "ok": true,
  "service": "websocket-chat"
}
```

## Test WebSocket with browser

Open:

```txt id="xnaj1r"
http://127.0.0.1:8080/
```

Expected behavior:

```txt id="kx7p5y"
browser connects to ws://127.0.0.1:9090/
server sends welcome message
browser joins room general
messages are broadcast to clients in the same room
```

## Test with websocat

If you have `websocat` installed:

```bash id="pqkx3n"
websocat ws://127.0.0.1:9090/
```

Send a raw message:

```txt id="eqipd5"
ping
```

Expected response:

```txt id="xakt6i"
pong
```

Send a typed room join message:

```json id="xyyqjc"
{ "type": "room.join", "payload": { "room": "general" } }
```

Expected response:

```txt id="v9j1mg"
joined room: general
```

Send a typed chat message:

```json id="e4iwwc"
{
  "type": "chat.message",
  "payload": { "room": "general", "text": "Hello from terminal" }
}
```

Expected response for clients in the room:

```txt id="l5mik6"
room general: Hello from terminal
```

## Typed message format

The browser sends messages like this:

```json id="ssx1i3"
{
  "type": "chat.message",
  "payload": {
    "room": "general",
    "text": "Hello"
  }
}
```

The important fields are:

```txt id="pnt4av"
type
  application event name

payload
  event data
```

For this example, the server handles:

```txt id="tw83tn"
room.join
room.leave
chat.message
```

## Raw messages vs typed messages

The server registers a raw message handler:

```cpp id="pljxvy"
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    if (message == "ping")
    {
      session.send_text("pong");
    }
  });
```

It also registers a typed message handler:

```cpp id="rqxh9i"
ws.on_typed_message(
  [&ws](vix::websocket::Session &session,
        const std::string &type,
        const vix::json::kvs &payload)
  {
    // handle typed JSON events
  });
```

Use raw messages for simple protocols.

Use typed messages for real application events.

A chat app should usually use typed messages.

## Rooms

Rooms let you group sessions.

The example joins a room with:

```cpp id="a37sug"
ws.join_room(session.shared_from_this(), room);
```

It leaves a room with:

```cpp id="a86w6z"
ws.leave_room(session.shared_from_this(), room);
```

It broadcasts to a room with:

```cpp id="biu240"
ws.broadcast_text_to_room(
  room,
  "room " + room + ": " + text
);
```

Rooms are useful for:

```txt id="dja7af"
chat rooms
support conversations
project channels
live dashboards
tenant-specific events
product pages
```

## Why `shared_from_this()` is used

Room APIs work with a shared session pointer.

Inside callbacks, the server gives you:

```cpp id="igap8y"
vix::websocket::Session &session
```

To join a room, use:

```cpp id="m2lixn"
session.shared_from_this()
```

Example:

```cpp id="iav22l"
ws.join_room(session.shared_from_this(), "general");
```

The server owns active sessions and room membership by shared session references.

## Attached runtime

This example uses:

```cpp id="buio97"
vix::websocket::AttachedRuntime runtime{
  app,
  ws,
  executor
};
```

This connects:

```txt id="ef31zm"
vix::App
vix::websocket::Server
RuntimeExecutor
```

The HTTP app and WebSocket server run as one application lifecycle.

The WebSocket server starts before the HTTP app enters `app.run(...)`.

When the app shuts down, the runtime coordinates WebSocket shutdown safely.

## Why use a shared executor

The WebSocket server needs a runtime executor:

```cpp id="serj86"
auto executor =
  std::make_shared<vix::executor::RuntimeExecutor>(4);
```

Then:

```cpp id="bq4ief"
vix::websocket::Server ws{config, executor};
```

and:

```cpp id="lre7u1"
vix::websocket::AttachedRuntime runtime{
  app,
  ws,
  executor
};
```

This keeps HTTP and WebSocket runtime coordination explicit.

## WebSocket configuration

The server reads configuration from:

```cpp id="p52zuw"
vix::config::Config config{".env"};
```

Important `.env` values:

```dotenv id="sxmgwa"
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

The browser connects to the configured port:

```js id="x2e8p5"
const socket = new WebSocket("ws://127.0.0.1:9090/");
```

If you change `WEBSOCKET_PORT`, update the browser URL too.

## HTTP and WebSocket ports

In this example:

```txt id="hxyc6z"
HTTP server
  8080

WebSocket server
  9090
```

That means:

```txt id="r9gkgd"
browser page
  http://127.0.0.1:8080/

WebSocket connection
  ws://127.0.0.1:9090/
```

In production, you may place both behind a reverse proxy.

The example keeps them separate to make the architecture clear.

## Avoid blocking inside callbacks

Do not run long blocking work directly inside WebSocket callbacks.

Avoid this:

```cpp id="s4hz41"
ws.on_message([](auto &, const std::string &)
{
  std::this_thread::sleep_for(std::chrono::seconds(10));
});
```

Callbacks should stay small.

For expensive work, dispatch to another service, queue, job system, or runtime task.

## Error handling

The example logs WebSocket errors:

```cpp id="f5z1w6"
ws.on_error(
  [](vix::websocket::Session &session, const std::string &error)
  {
    (void)session;

    vix::print("websocket error:", error);
  });
```

Use this for:

```txt id="ftzpvg"
connection diagnostics
invalid frames
client disconnect issues
read failures
write failures
```

## Security notes

This is a demo.

For a real chat system, add:

```txt id="u0jrg6"
authentication
room authorization
message size limits
rate limiting
input validation
message persistence
presence state
origin checks
TLS through reverse proxy
```

Do not allow any client to join any room in production unless that is your intended design.

## Common mistakes

### Using HTTP port for WebSocket

This example uses:

```txt id="jvr8af"
HTTP      8080
WebSocket 9090
```

So this is correct:

```js id="vkxtcm"
new WebSocket("ws://127.0.0.1:9090/");
```

This is wrong for this example:

```js id="q5fdh3"
new WebSocket("ws://127.0.0.1:8080/");
```

unless you configured WebSocket to run on the same port through another integration.

### Forgetting the executor

The server needs a runtime executor:

```cpp id="e1l99a"
auto executor =
  std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::Server ws{config, executor};
```

### Blocking shutdown incorrectly

Do not manually block inside shutdown callbacks.

Use `AttachedRuntime` to coordinate HTTP and WebSocket lifecycle.

### Treating raw messages as typed messages

Raw messages are plain strings.

Typed messages are JSON objects with:

```txt id="vfq1mg"
type
payload
```

Use `on_typed_message(...)` for typed events.

## Complete test flow

Run:

```bash id="ez3haw"
vix run websocket_chat.cpp
```

Open:

```txt id="iwcjdb"
http://127.0.0.1:8080/
```

Open two browser tabs.

In both tabs:

```txt id="e00ihk"
room = general
click Join room
```

In one tab, send:

```txt id="xm1ucz"
Hello from Vix
```

Expected behavior:

```txt id="ynrwaq"
both clients in the general room receive the message
```

Test raw ping with `websocat`:

```bash id="abh88v"
websocat ws://127.0.0.1:9090/
```

Send:

```txt id="el6bmd"
ping
```

Expected response:

```txt id="cjs6ri"
pong
```

## Summary

A minimal Vix WebSocket chat needs:

```cpp id="f74339"
vix::config::Config config{".env"};

auto executor =
  std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::Server ws{config, executor};
```

Then register callbacks:

```cpp id="ldm9gv"
ws.on_open(...);
ws.on_close(...);
ws.on_error(...);
ws.on_message(...);
ws.on_typed_message(...);
```

Use rooms for grouped broadcasts:

```cpp id="qykm8s"
ws.join_room(session.shared_from_this(), "general");
ws.broadcast_text_to_room("general", "message");
```

Use `AttachedRuntime` when HTTP and WebSocket run together:

```cpp id="g6xq7j"
vix::websocket::AttachedRuntime runtime{
  app,
  ws,
  executor
};
```

The mental model is:

```txt id="srpuxz"
vix::App
  serves HTTP routes and static files

vix::websocket::Server
  handles realtime connections

AttachedRuntime
  coordinates lifecycle

rooms
  group sessions

typed messages
  carry application events
```
