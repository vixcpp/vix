# API Reference

This page summarizes the public API of the Vix WebSocket module.

Use it when you want a quick reference for headers, classes, callbacks, configuration, server APIs, client APIs, sessions, long-polling, metrics, persistence, OpenAPI, and attached HTTP + WebSocket runtime helpers.

## Main header

Use the umbrella header for most applications:

```cpp
#include <vix/websocket.hpp>
```

This includes the main WebSocket APIs.

## Direct headers

```cpp
#include <vix/websocket/config.hpp>
#include <vix/websocket/protocol.hpp>
#include <vix/websocket/client.hpp>
#include <vix/websocket/server.hpp>
#include <vix/websocket/session.hpp>
#include <vix/websocket/router.hpp>
#include <vix/websocket/MessageStore.hpp>
#include <vix/websocket/SqliteMessageStore.hpp>
#include <vix/websocket/Metrics.hpp>
#include <vix/websocket/App.hpp>
#include <vix/websocket/HttpApi.hpp>
#include <vix/websocket/LongPolling.hpp>
#include <vix/websocket/LongPollingBridge.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
#include <vix/websocket/Runtime.hpp>
```

## Namespace

The main namespace is:

```cpp
namespace vix::websocket
```

Some combined runtime helpers live in:

```cpp
namespace vix
```

## Public API overview

| API                     | Purpose                                            |
| ----------------------- | -------------------------------------------------- |
| `Config`                | WebSocket runtime configuration.                   |
| `JsonMessage`           | Typed JSON message model.                          |
| `Server`                | High-level WebSocket server.                       |
| `LowLevelServer`        | Low-level native async WebSocket engine.           |
| `Session`               | One connected WebSocket client.                    |
| `Router`                | Dispatches open, close, error, and message events. |
| `Client`                | Native WebSocket client.                           |
| `LongPollingSession`    | In-memory long-polling session buffer.             |
| `LongPollingManager`    | Thread-safe manager for polling sessions.          |
| `LongPollingBridge`     | Bridge between WebSocket and long-polling.         |
| `WebSocketMetrics`      | Prometheus-style metrics counters and gauges.      |
| `IMessageStore`         | Abstract message persistence interface.            |
| `SqliteMessageStore`    | SQLite-backed message store.                       |
| `AttachedRuntime`       | Runs WebSocket beside a `vix::App`.                |
| `register_ws_docs(...)` | Registers OpenAPI docs for WebSocket endpoints.    |

## Config

Header:

```cpp
#include <vix/websocket/config.hpp>
```

Type:

```cpp
vix::websocket::Config
```

Fields:

```cpp
std::size_t maxMessageSize = 64 * 1024;
std::chrono::seconds idleTimeout{60};
bool enablePerMessageDeflate = true;
bool autoPingPong = true;
std::chrono::seconds pingInterval{30};
```

Static functions:

```cpp
static Config from_core(const vix::config::Config &core);
```

Example:

```cpp
vix::config::Config core{".env"};

vix::websocket::Config ws =
    vix::websocket::Config::from_core(core);
```

## Configuration keys

Common environment values:

```dotenv
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

## Server

Header:

```cpp
#include <vix/websocket/server.hpp>
```

Type:

```cpp
vix::websocket::Server
```

Purpose:

```txt
High-level WebSocket server with sessions, callbacks, rooms, broadcasting,
typed messages, shutdown, and optional long-polling bridge integration.
```

## Server construction

```cpp
Server(
    vix::config::Config &cfg,
    std::shared_ptr<vix::executor::RuntimeExecutor> executor);
```

```cpp
Server(
    vix::config::Config &cfg,
    std::unique_ptr<vix::executor::RuntimeExecutor> executor);
```

Example:

```cpp
vix::config::Config config{".env"};

auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::Server ws{config, executor};
```

## Server callback types

```cpp
using OpenHandler =
    std::function<void(Session &)>;

using CloseHandler =
    std::function<void(Session &)>;

using ErrorHandler =
    std::function<void(Session &, const std::string &)>;

using MessageHandler =
    std::function<void(Session &, const std::string &)>;

using TypedMessageHandler =
    std::function<void(Session &, const std::string &, const vix::json::kvs &)>;
```

## Server callbacks

```cpp
void on_open(OpenHandler fn);
void on_close(CloseHandler fn);
void on_error(ErrorHandler fn);
void on_message(MessageHandler fn);
void on_typed_message(TypedMessageHandler fn);
```

Example:

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("echo: " + message);
  });
```

Typed example:

```cpp
ws.on_typed_message(
  [](vix::websocket::Session &session,
     const std::string &type,
     const vix::json::kvs &payload)
  {
    (void)payload;

    if (type == "ping")
    {
      session.send_text("pong");
    }
  });
```

## Server lifecycle

```cpp
void start();
void stop_async();
void stop();
void listen_blocking();
```

Use:

```cpp
ws.start();
```

For shutdown:

```cpp
ws.stop_async();
ws.stop();
```

`stop_async()` requests non-blocking shutdown.

`stop()` performs final blocking shutdown and joins internal server threads.

## Server rooms

```cpp
void join_room(std::shared_ptr<Session> session, const RoomId &room);
void leave_room(std::shared_ptr<Session> session, const RoomId &room);
```

Example:

```cpp
ws.on_open([&ws](vix::websocket::Session &session)
{
  ws.join_room(session.shared_from_this(), "chat:general");
});
```

## Server broadcasting

```cpp
void broadcast_text(const std::string &text);
void broadcast_text_to_room(const RoomId &room, const std::string &text);
```

Examples:

```cpp
ws.broadcast_text("global message");
ws.broadcast_text_to_room("chat:general", "room message");
```

## Server long-polling bridge

```cpp
void attach_long_polling_bridge(LongPollingBridge *bridge);
```

Example:

```cpp
vix::websocket::LongPollingBridge bridge{
    &metrics,
    std::chrono::seconds{60},
    256};

ws.attach_long_polling_bridge(&bridge);
```

## Server information

Common server information APIs include:

```cpp
int port() const;
int bound_port() const;
```

Use these to inspect the configured or bound WebSocket port when supported by the current build.

## LowLevelServer

Header:

```cpp
#include <vix/websocket/websocket.hpp>
```

Type:

```cpp
vix::websocket::LowLevelServer
```

Purpose:

```txt
Low-level asynchronous WebSocket server engine.
```

It owns:

- async I/O context
- TCP listener
- accept loop
- I/O worker threads
- session creation
- server shutdown state

Most applications should use `vix::websocket::Server` instead.

## LowLevelServer API

```cpp
LowLevelServer(
    vix::config::Config &coreConfig,
    std::shared_ptr<vix::executor::RuntimeExecutor> executor,
    std::shared_ptr<Router> router);

~LowLevelServer();

void run();
void stop_async();
void join_threads();

bool is_stop_requested() const;
```

## Session

Header:

```cpp
#include <vix/websocket/session.hpp>
```

Type:

```cpp
vix::websocket::Session
```

Purpose:

```txt
Represents one connected WebSocket client.
```

A session manages:

- HTTP Upgrade handshake
- frame reads
- frame writes
- text messages
- binary messages
- close handling
- heartbeat
- idle timeout
- router dispatch

## Session construction

```cpp
Session(
    std::unique_ptr<tcp_stream> stream,
    const Config &cfg,
    std::shared_ptr<Router> router,
    std::shared_ptr<vix::executor::RuntimeExecutor> executor,
    std::shared_ptr<io_context> ioc);
```

Most applications do not construct sessions directly.

The low-level server creates them internally.

## Session API

```cpp
task<void> run();

void send_text(std::string_view text);
void send_binary(const void *data, std::size_t size);

void close(std::string reason = {});
bool is_open() const noexcept;

void emit_error(const std::string &message);
void shutdown_now() noexcept;
```

Example:

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    if (session.is_open())
    {
      session.send_text("received: " + message);
    }
  });
```

## Router

Header:

```cpp
#include <vix/websocket/router.hpp>
```

Type:

```cpp
vix::websocket::Router
```

Purpose:

```txt
Lightweight event dispatcher for WebSocket sessions.
```

## Router handler types

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

## Router registration API

```cpp
void on_open(OpenHandler cb);
void on_close(CloseHandler cb);
void on_error(ErrorHandler cb);
void on_message(MessageHandler cb);
```

## Router dispatch API

```cpp
void handle_open(Session &session) const;
void handle_close(Session &session) const;
void handle_error(Session &session, const std::string &error) const;
void handle_message(Session &session, std::string payload) const;
```

## Router inspection API

```cpp
bool has_open_handler() const noexcept;
bool has_close_handler() const noexcept;
bool has_error_handler() const noexcept;
bool has_message_handler() const noexcept;
```

Most applications should register callbacks through `Server` instead of using `Router` directly.

## Client

Header:

```cpp
#include <vix/websocket/client.hpp>
```

Type:

```cpp
vix::websocket::Client
```

Purpose:

```txt
Native WebSocket client with async connect, text messages, typed JSON messages,
heartbeat, and optional reconnect.
```

The client exposes callback setters, connection control, text sending, typed JSON sending, ping, close, and connection state helpers. :contentReference[oaicite:0]{index=0}

## Client construction

```cpp
static std::shared_ptr<Client> create(
    std::string host,
    std::string port,
    std::string target = "/");
```

Example:

```cpp
auto client =
    vix::websocket::Client::create("127.0.0.1", "9090", "/");
```

## Client callback types

```cpp
using OpenHandler =
    std::function<void()>;

using MessageHandler =
    std::function<void(const std::string &)>;

using CloseHandler =
    std::function<void()>;

using ErrorHandler =
    std::function<void(const std::string &)>;
```

## Client callbacks

```cpp
void on_open(OpenHandler cb);
void on_message(MessageHandler cb);
void on_close(CloseHandler cb);
void on_error(ErrorHandler cb);
```

Example:

```cpp
client->on_open([client]()
{
  client->send_text("hello");
});

client->on_message([](const std::string &message)
{
  vix::print("received:", message);
});
```

## Client connection API

```cpp
void connect();
void close();

bool is_connected() const noexcept;
```

`connect()` starts the client runtime thread and async connection flow.

`close()` closes the connection, stops runtime activity, and joins client threads when safe.

## Client sending API

```cpp
void send_text(const std::string &text);

void send_json_message(
    const std::string &type,
    const vix::json::kvs &payload);

void send_json_message(
    const std::string &type,
    std::initializer_list<vix::json::token> payloadTokens);

void send(
    const std::string &type,
    const vix::json::kvs &payload);

void send(
    const std::string &type,
    std::initializer_list<vix::json::token> payloadTokens);

void send_ping();
```

Example:

```cpp
client->send_json_message(
    "chat.message",
    {
      {"text", "Hello"}
    });
```

The `send(...)` overloads are convenience aliases for typed JSON sending.

## Client heartbeat and reconnect

```cpp
void enable_heartbeat(std::chrono::seconds interval);

void enable_auto_reconnect(
    bool enable,
    std::chrono::seconds delay = std::chrono::seconds{3});
```

Example:

```cpp
client->enable_heartbeat(std::chrono::seconds{30});
client->enable_auto_reconnect(true, std::chrono::seconds{3});
```

## JsonMessage

Header:

```cpp
#include <vix/websocket/protocol.hpp>
```

Type:

```cpp
vix::websocket::JsonMessage
```

Purpose:

```txt
Structured typed WebSocket message model.
```

Common fields:

```cpp
std::string id;
std::string kind;
std::string room;
std::string type;
std::string ts;
vix::json::kvs payload;
```

Common shape:

```json
{
  "id": "00000000000000000001",
  "kind": "event",
  "room": "general",
  "type": "chat.message",
  "ts": "2026-05-17T10:00:00Z",
  "payload": {
    "text": "Hello"
  }
}
```

## JsonMessage API

Common API:

```cpp
static std::optional<JsonMessage> parse(const std::string &text);

std::string to_json_string() const;
```

Example:

```cpp
auto parsed = vix::websocket::JsonMessage::parse(text);

if (parsed)
{
  vix::print("type:", parsed->type);
}
```

## Protocol detail API

Header:

```cpp
#include <vix/websocket/protocol.hpp>
```

The protocol layer includes internal frame helpers under:

```cpp
namespace vix::websocket::detail
```

Important internal types include:

```cpp
enum class Opcode;
struct Frame;
struct FrameHeader;
```

Frame helpers are used by the client and session implementation.

Most applications should use `Server`, `Session`, `Client`, and `JsonMessage` instead of protocol internals.

## LongPollingSession

Header:

```cpp
#include <vix/websocket/LongPolling.hpp>
```

Type:

```cpp
vix::websocket::LongPollingSession
```

Fields:

```cpp
std::string id;
std::chrono::steady_clock::time_point lastSeen;
std::deque<JsonMessage> buffer;
```

API:

```cpp
LongPollingSession();
explicit LongPollingSession(std::string sessionId);

void touch() noexcept;

bool is_expired(
    std::chrono::seconds ttl,
    std::chrono::steady_clock::time_point now) const noexcept;

void enqueue(const JsonMessage &msg, std::size_t maxBufferSize);

std::vector<JsonMessage> drain(std::size_t maxCount);
```

The session stores a bounded FIFO buffer and refreshes `lastSeen` when messages are enqueued or drained. :contentReference[oaicite:1]{index=1}

## LongPollingManager

Header:

```cpp
#include <vix/websocket/LongPolling.hpp>
```

Type:

```cpp
vix::websocket::LongPollingManager
```

Purpose:

```txt
Thread-safe manager for long-polling sessions and buffers.
```

## LongPollingManager construction

```cpp
LongPollingManager(
    std::chrono::seconds sessionTtl = std::chrono::seconds{60},
    std::size_t maxBufferPerSession = 256,
    WebSocketMetrics *metrics = nullptr);
```

Example:

```cpp
vix::websocket::LongPollingManager manager{
    std::chrono::seconds{60},
    256,
    &metrics};
```

## LongPollingManager API

```cpp
void push_to(const SessionId &sessionId, const JsonMessage &message);

std::vector<JsonMessage> poll(
    const SessionId &sessionId,
    std::size_t maxMessages = 50,
    bool createIfMissing = true);

void sweep_expired();

std::size_t session_count() const;

std::size_t buffer_size(const SessionId &sessionId) const;
```

`push_to(...)` creates the session if missing, `poll(...)` drains up to `maxMessages`, and `sweep_expired()` removes inactive sessions based on TTL. :contentReference[oaicite:2]{index=2}

## LongPollingBridge

Header:

```cpp
#include <vix/websocket/LongPollingBridge.hpp>
```

Type:

```cpp
vix::websocket::LongPollingBridge
```

Purpose:

```txt
Bridge between WebSocket JsonMessage events and HTTP long-polling sessions.
```

## LongPollingBridge aliases

```cpp
using SessionId = std::string;

using Resolver =
    std::function<SessionId(const JsonMessage &)>;

using HttpToWsForward =
    std::function<void(const JsonMessage &)>;
```

## LongPollingBridge constructors

With external manager:

```cpp
LongPollingBridge(
    LongPollingManager &manager,
    Resolver resolver = {},
    HttpToWsForward httpToWs = {});
```

Owning manager:

```cpp
LongPollingBridge(
    WebSocketMetrics *metrics,
    std::chrono::seconds sessionTtl = std::chrono::seconds{60},
    std::size_t maxBufferPerSession = 256,
    Resolver resolver = {},
    HttpToWsForward httpToWs = {});
```

## LongPollingBridge API

```cpp
void on_ws_message(const JsonMessage &msg);

std::vector<JsonMessage> poll(
    const SessionId &sessionId,
    std::size_t maxMessages = 50,
    bool createIfMissing = true);

void send_from_http(
    const SessionId &sessionId,
    const JsonMessage &msg);

LongPollingManager &manager() noexcept;
const LongPollingManager &manager() const noexcept;

std::size_t session_count() const;
std::size_t buffer_size(const SessionId &sid) const;
```

Default resolution:

```txt
if msg.room is not empty:
  session id = "room:" + msg.room
else:
  session id = "broadcast"
```

## WebSocketMetrics

Header:

```cpp
#include <vix/websocket/Metrics.hpp>
```

Type:

```cpp
vix::websocket::WebSocketMetrics
```

Purpose:

```txt
Prometheus-style metrics for WebSocket and long-polling.
```

## WebSocketMetrics fields

```cpp
std::atomic<std::uint64_t> connections_total{0};
std::atomic<std::uint64_t> connections_active{0};

std::atomic<std::uint64_t> messages_in_total{0};
std::atomic<std::uint64_t> messages_out_total{0};

std::atomic<std::uint64_t> errors_total{0};

std::atomic<std::uint64_t> lp_sessions_total{0};
std::atomic<std::uint64_t> lp_sessions_active{0};

std::atomic<std::uint64_t> lp_polls_total{0};

std::atomic<std::uint64_t> lp_messages_buffered{0};
std::atomic<std::uint64_t> lp_messages_enqueued_total{0};
std::atomic<std::uint64_t> lp_messages_drained_total{0};
```

## WebSocketMetrics API

```cpp
std::string render_prometheus() const;
```

Example:

```cpp
vix::websocket::WebSocketMetrics metrics;

metrics.connections_total.fetch_add(1, std::memory_order_relaxed);

std::string text = metrics.render_prometheus();
```

## Metrics exporter

```cpp
void run_metrics_http_exporter(
    WebSocketMetrics &metrics,
    const std::string &address = "0.0.0.0",
    std::uint16_t port = 9100);
```

Example:

```cpp
vix::websocket::run_metrics_http_exporter(
    metrics,
    "0.0.0.0",
    9100);
```

The exporter serves Prometheus text at:

```txt
GET /metrics
```

The exporter reads an HTTP request head, returns `metrics.render_prometheus()` for `/metrics`, and returns `404` for other targets. :contentReference[oaicite:3]{index=3}

## IMessageStore

Header:

```cpp
#include <vix/websocket/MessageStore.hpp>
```

Type:

```cpp
vix::websocket::IMessageStore
```

Purpose:

```txt
Abstract persistence interface for JsonMessage history and replay.
```

## IMessageStore API

```cpp
virtual ~IMessageStore() = default;

virtual void append(const JsonMessage &msg) = 0;

virtual std::vector<JsonMessage> list_by_room(
    const std::string &room,
    std::size_t limit,
    const std::optional<std::string> &before_id = std::nullopt) = 0;

virtual std::vector<JsonMessage> replay_from(
    const std::string &start_id,
    std::size_t limit) = 0;
```

## SqliteMessageStore

Header:

```cpp
#include <vix/websocket/SqliteMessageStore.hpp>
```

Type:

```cpp
vix::websocket::SqliteMessageStore
```

Purpose:

```txt
SQLite-backed implementation of IMessageStore.
```

## SqliteMessageStore construction

```cpp
explicit SqliteMessageStore(const std::string &db_path);
```

Example:

```cpp
vix::websocket::SqliteMessageStore store{"messages.db"};
```

## SqliteMessageStore API

```cpp
~SqliteMessageStore() override;

void append(const JsonMessage &msg) override;

std::vector<JsonMessage> list_by_room(
    const std::string &room,
    std::size_t limit,
    const std::optional<std::string> &before_id = std::nullopt) override;

std::vector<JsonMessage> replay_from(
    const std::string &start_id,
    std::size_t limit) override;
```

The SQLite store opens or creates the database, enables WAL mode, creates the `messages` table, and stores fields such as `id`, `kind`, `room`, `type`, `ts`, and `payload_json`.

## App

Header:

```cpp
#include <vix/websocket/App.hpp>
```

Type:

```cpp
vix::websocket::App
```

Purpose:

```txt
High-level WebSocket application wrapper.
```

It manages:

- config
- executor
- underlying WebSocket server
- typed message routes
- shutdown

## App handler type

```cpp
using TypedHandler =
    std::function<void(
        Session &,
        const std::string &type,
        const vix::json::kvs &payload)>;
```

## App construction

```cpp
App(
    const std::string &configPath,
    std::shared_ptr<vix::executor::RuntimeExecutor> executor);
```

Example:

```cpp
auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::App app{".env", executor};
```

## App API

```cpp
App &ws(const std::string &endpoint, TypedHandler handler);

void run_blocking();
void stop() noexcept;

Server &server() noexcept;
vix::config::Config &config() noexcept;

std::shared_ptr<vix::executor::RuntimeExecutor> executor() noexcept;
```

## AttachedRuntime

Header:

```cpp
#include <vix/websocket/AttachedRuntime.hpp>
```

Type:

```cpp
vix::websocket::AttachedRuntime
```

Purpose:

```txt
Attach a WebSocket server to a vix::App with shared lifecycle.
```

## AttachedRuntime construction

```cpp
AttachedRuntime(
    vix::App &app,
    vix::websocket::Server &ws,
    std::shared_ptr<vix::executor::RuntimeExecutor> exec);
```

Example:

```cpp
vix::App app;

auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::Server ws{app.config(), executor};

vix::websocket::AttachedRuntime runtime{app, ws, executor};
```

The constructor starts the WebSocket server and registers an HTTP shutdown callback. :contentReference[oaicite:4]{index=4}

## AttachedRuntime API

```cpp
void request_stop() noexcept;
void finalize_shutdown() noexcept;
```

`request_stop()` requests non-blocking WebSocket shutdown.

`finalize_shutdown()` performs final blocking cleanup by stopping the WebSocket server and then stopping the shared executor. :contentReference[oaicite:5]{index=5}

## Combined runtime helpers

Header:

```cpp
#include <vix/websocket/AttachedRuntime.hpp>
```

Namespace:

```cpp
namespace vix
```

## register_ws_openapi_docs_once

```cpp
void register_ws_openapi_docs_once();
```

Registers WebSocket OpenAPI docs once per process.

Default documented routes:

```txt
GET  /ws
GET  /ws/poll
POST /ws/send
GET  /metrics
```

## run_http_and_ws

```cpp
void run_http_and_ws(
    vix::App &app,
    vix::websocket::Server &ws,
    std::shared_ptr<vix::executor::RuntimeExecutor> exec,
    const vix::config::Config &cfg);
```

Port overload:

```cpp
void run_http_and_ws(
    vix::App &app,
    vix::websocket::Server &ws,
    std::shared_ptr<vix::executor::RuntimeExecutor> exec,
    int port);
```

Purpose:

```txt
Run HTTP and WebSocket together with shared config and executor.
```

The combined runner registers WebSocket docs, HTTP OpenAPI routes, Swagger UI routes, creates an `AttachedRuntime`, then runs the HTTP app. :contentReference[oaicite:6]{index=6}

## serve_http_and_ws

```cpp
serve_http_and_ws(configPath, port, fn);
serve_http_and_ws(fn);
```

Purpose:

```txt
High-level helper that creates Config, RuntimeExecutor, App, and WebSocket Server,
then lets the caller configure HTTP and WebSocket behavior.
```

Example:

```cpp
vix::serve_http_and_ws([](vix::App &app, vix::websocket::Server &ws)
{
  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("HTTP + WebSocket");
  });

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      session.send_text("echo: " + message);
    });
});
```

## OpenAPI docs

Header:

```cpp
#include <vix/websocket/openapi_docs.hpp>
```

Namespace:

```cpp
namespace vix::websocket::openapi
```

Function:

```cpp
void register_ws_docs(
    std::string ws_upgrade_path = "/ws",
    std::string lp_poll_path = "/ws/poll",
    std::string lp_send_path = "/ws/send",
    std::string metrics_path = "/metrics");
```

Example:

```cpp
vix::websocket::openapi::register_ws_docs();
```

Custom paths:

```cpp
vix::websocket::openapi::register_ws_docs(
    "/api/ws",
    "/api/ws/poll",
    "/api/ws/send",
    "/api/ws/metrics");
```

## HTTP API helpers

Header:

```cpp
#include <vix/websocket/HttpApi.hpp>
```

Namespace:

```cpp
namespace vix::websocket::http
```

Purpose:

```txt
Helpers for long-polling HTTP endpoints and request parsing.
```

Common endpoints:

```txt
GET  /ws/poll
POST /ws/send
```

The helper code supports URL decoding, query parameter extraction, request target extraction, and integration with `LongPollingBridge`.

## Minimal server example

```cpp
#include <memory>
#include <string>

#include <vix/config/Config.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config config{".env"};

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{config, executor};

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      session.send_text("echo: " + message);
    });

  ws.start();

  return 0;
}
```

## Minimal client example

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
    client->send_text("hello");
  });

  client->on_message([](const std::string &message)
  {
    vix::print("received:", message);
  });

  client->connect();

  return 0;
}
```

## Minimal attached runtime example

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

  app.run(8080);

  return 0;
}
```

## API by use case

| Use case                  | API                                       |
| ------------------------- | ----------------------------------------- |
| Start WebSocket server    | `vix::websocket::Server`                  |
| Handle connection open    | `Server::on_open(...)`                    |
| Handle raw message        | `Server::on_message(...)`                 |
| Handle typed JSON message | `Server::on_typed_message(...)`           |
| Reply to one client       | `Session::send_text(...)`                 |
| Send binary payload       | `Session::send_binary(...)`               |
| Close one client          | `Session::close(...)`                     |
| Broadcast to all clients  | `Server::broadcast_text(...)`             |
| Broadcast to room         | `Server::broadcast_text_to_room(...)`     |
| Join room                 | `Server::join_room(...)`                  |
| Leave room                | `Server::leave_room(...)`                 |
| Create C++ client         | `Client::create(...)`                     |
| Send client text          | `Client::send_text(...)`                  |
| Send client typed JSON    | `Client::send_json_message(...)`          |
| Enable heartbeat          | `Client::enable_heartbeat(...)`           |
| Enable reconnect          | `Client::enable_auto_reconnect(...)`      |
| Use long-polling          | `LongPollingManager`, `LongPollingBridge` |
| Export metrics            | `WebSocketMetrics::render_prometheus()`   |
| Run metrics exporter      | `run_metrics_http_exporter(...)`          |
| Store messages            | `IMessageStore`                           |
| Store in SQLite           | `SqliteMessageStore`                      |
| Attach HTTP + WebSocket   | `AttachedRuntime`                         |
| Register OpenAPI docs     | `register_ws_docs(...)`                   |

## Related pages

- [Quick Start](./quick-start.md)
- [Server](./server.md)
- [Session](./session.md)
- [Messages](./messages.md)
- [Client](./client.md)
- [Long polling](./long-polling.md)
- [Attached runtime](./attached-runtime.md)
- [Shutdown](./shutdown.md)
