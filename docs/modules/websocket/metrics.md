# Metrics

This page explains `vix::websocket::WebSocketMetrics`.

Use it when you want to observe WebSocket connections, messages, errors, long-polling sessions, polling activity, and buffered fallback messages.

## Header

```cpp
#include <vix/websocket/Metrics.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What metrics provide

The WebSocket module provides lightweight Prometheus-style metrics.

Metrics help you observe:

- accepted WebSocket connections
- active WebSocket connections
- inbound messages
- outbound messages
- errors
- long-polling sessions
- long-polling poll calls
- buffered long-polling messages
- enqueued long-polling messages
- drained long-polling messages

The main type is:

```cpp
vix::websocket::WebSocketMetrics
```

## Basic usage

```cpp
#include <vix/print.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::websocket::WebSocketMetrics metrics;

  metrics.connections_total.fetch_add(1);
  metrics.connections_active.fetch_add(1);
  metrics.messages_in_total.fetch_add(1);

  vix::print(metrics.render_prometheus());

  return 0;
}
```

## WebSocketMetrics

`WebSocketMetrics` is a struct of atomic counters and gauges.

```cpp
vix::websocket::WebSocketMetrics metrics;
```

The fields are atomic so they can be updated from multiple WebSocket or long-polling paths.

The metrics type can render itself in Prometheus text exposition format:

```cpp
std::string output = metrics.render_prometheus();
```

## Connection metrics

| Metric               | Type    | Meaning                                             |
| -------------------- | ------- | --------------------------------------------------- |
| `connections_total`  | Counter | Total accepted WebSocket connections since startup. |
| `connections_active` | Gauge   | Current number of active WebSocket connections.     |

Example:

```cpp
metrics.connections_total.fetch_add(1, std::memory_order_relaxed);
metrics.connections_active.fetch_add(1, std::memory_order_relaxed);
```

When a connection closes:

```cpp
metrics.connections_active.fetch_sub(1, std::memory_order_relaxed);
```

## Message metrics

| Metric               | Type    | Meaning                                    |
| -------------------- | ------- | ------------------------------------------ |
| `messages_in_total`  | Counter | Total inbound messages processed.          |
| `messages_out_total` | Counter | Total outbound messages sent or broadcast. |

Example:

```cpp
metrics.messages_in_total.fetch_add(1, std::memory_order_relaxed);
metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);
```

Use these metrics to track realtime traffic volume.

## Error metrics

| Metric         | Type    | Meaning                                          |
| -------------- | ------- | ------------------------------------------------ |
| `errors_total` | Counter | Total WebSocket or long-polling errors observed. |

Example:

```cpp
metrics.errors_total.fetch_add(1, std::memory_order_relaxed);
```

Use this when:

- a session emits an error
- a frame fails validation
- a polling request fails
- a bridge operation fails
- a metrics exporter observes an error

## Long-polling session metrics

| Metric               | Type    | Meaning                               |
| -------------------- | ------- | ------------------------------------- |
| `lp_sessions_total`  | Counter | Total long-polling sessions created.  |
| `lp_sessions_active` | Gauge   | Current active long-polling sessions. |

The long-polling manager can update these when sessions are created or removed.

```cpp
metrics.lp_sessions_total.fetch_add(1, std::memory_order_relaxed);
metrics.lp_sessions_active.fetch_add(1, std::memory_order_relaxed);
```

## Long-polling poll metrics

| Metric           | Type    | Meaning                               |
| ---------------- | ------- | ------------------------------------- |
| `lp_polls_total` | Counter | Total long-polling poll calls served. |

Each call to poll can increment this value.

```cpp
metrics.lp_polls_total.fetch_add(1, std::memory_order_relaxed);
```

This helps you understand how often HTTP fallback clients are polling.

## Long-polling buffer metrics

| Metric                       | Type       | Meaning                                              |
| ---------------------------- | ---------- | ---------------------------------------------------- |
| `lp_messages_buffered`       | Gauge-like | Current number of buffered messages across sessions. |
| `lp_messages_enqueued_total` | Counter    | Total messages enqueued into long-polling buffers.   |
| `lp_messages_drained_total`  | Counter    | Total messages drained from long-polling buffers.    |

These metrics help you detect whether fallback clients are keeping up.

If `lp_messages_buffered` grows continuously, clients may not be polling fast enough.

## Prometheus rendering

Use `render_prometheus()` to export metrics as text.

```cpp
vix::websocket::WebSocketMetrics metrics;

std::string text = metrics.render_prometheus();
```

Example output shape:

```txt
# HELP vix_ws_connections_total Total WebSocket connections.
# TYPE vix_ws_connections_total counter
vix_ws_connections_total 12

# HELP vix_ws_connections_active Active WebSocket connections.
# TYPE vix_ws_connections_active gauge
vix_ws_connections_active 3
```

The exact names and output format are controlled by the implementation.

## Metrics exporter

The module provides a minimal HTTP metrics exporter.

```cpp
vix::websocket::run_metrics_http_exporter(
    metrics,
    "0.0.0.0",
    9100);
```

This exposes metrics at:

```txt
http://localhost:9100/metrics
```

Signature:

```cpp
void run_metrics_http_exporter(
    WebSocketMetrics &metrics,
    const std::string &address = "0.0.0.0",
    std::uint16_t port = 9100);
```

## Minimal exporter example

```cpp
#include <vix/websocket.hpp>

int main()
{
  vix::websocket::WebSocketMetrics metrics;

  vix::websocket::run_metrics_http_exporter(
      metrics,
      "0.0.0.0",
      9100);

  return 0;
}
```

This function is blocking.

Run it from a dedicated control path or thread if your application also needs to run other servers.

## Exporter endpoint

The exporter serves:

```txt
GET /metrics
```

Requests to `/metrics` return the Prometheus text output from:

```cpp
metrics.render_prometheus()
```

This can be scraped by Prometheus or any compatible monitoring system.

## Dedicated exporter thread

If your main application already runs HTTP and WebSocket, run the metrics exporter in a separate thread.

```cpp
#include <thread>

#include <vix/websocket.hpp>

int main()
{
  vix::websocket::WebSocketMetrics metrics;

  std::thread metricsThread([&metrics]()
  {
    vix::websocket::run_metrics_http_exporter(
        metrics,
        "0.0.0.0",
        9100);
  });

  metricsThread.join();

  return 0;
}
```

In a real application, coordinate shutdown around the exporter thread.

## Metrics with WebSocket server

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

  vix::websocket::WebSocketMetrics metrics;

  vix::websocket::Server ws{config, executor};

  ws.on_open([&metrics](vix::websocket::Session &session)
  {
    (void)session;

    metrics.connections_total.fetch_add(1, std::memory_order_relaxed);
    metrics.connections_active.fetch_add(1, std::memory_order_relaxed);
  });

  ws.on_close([&metrics](vix::websocket::Session &session)
  {
    (void)session;

    metrics.connections_active.fetch_sub(1, std::memory_order_relaxed);
  });

  ws.on_message(
    [&metrics](vix::websocket::Session &session, const std::string &message)
    {
      metrics.messages_in_total.fetch_add(1, std::memory_order_relaxed);

      session.send_text("echo: " + message);

      metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);
    });

  ws.on_error(
    [&metrics](vix::websocket::Session &session, const std::string &error)
    {
      (void)session;
      (void)error;

      metrics.errors_total.fetch_add(1, std::memory_order_relaxed);
    });

  ws.start();

  return 0;
}
```

## Metrics with long-polling

`LongPollingManager` can receive a metrics pointer.

```cpp
vix::websocket::WebSocketMetrics metrics;

vix::websocket::LongPollingManager manager{
    std::chrono::seconds{60},
    256,
    &metrics};
```

The manager can update metrics when:

- sessions are created
- poll calls are served
- messages are enqueued
- messages are drained
- sessions expire

## Metrics with LongPollingBridge

```cpp
vix::websocket::WebSocketMetrics metrics;

vix::websocket::LongPollingBridge bridge{
    &metrics,
    std::chrono::seconds{60},
    256};
```

Attach the bridge to the WebSocket server:

```cpp
ws.attach_long_polling_bridge(&bridge);
```

Then typed WebSocket messages can be reflected into long-polling buffers, while the long-polling manager updates fallback metrics.

## Full example with exporter

```cpp
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <vix/config/Config.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config config{".env"};

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::WebSocketMetrics metrics;

  std::thread metricsThread([&metrics]()
  {
    vix::websocket::run_metrics_http_exporter(
        metrics,
        "0.0.0.0",
        9100);
  });

  vix::websocket::LongPollingBridge bridge{
      &metrics,
      std::chrono::seconds{60},
      256};

  vix::websocket::Server ws{config, executor};

  ws.attach_long_polling_bridge(&bridge);

  ws.on_open([&metrics](vix::websocket::Session &session)
  {
    (void)session;

    metrics.connections_total.fetch_add(1, std::memory_order_relaxed);
    metrics.connections_active.fetch_add(1, std::memory_order_relaxed);
  });

  ws.on_close([&metrics](vix::websocket::Session &session)
  {
    (void)session;

    metrics.connections_active.fetch_sub(1, std::memory_order_relaxed);
  });

  ws.on_message(
    [&metrics](vix::websocket::Session &session, const std::string &message)
    {
      metrics.messages_in_total.fetch_add(1, std::memory_order_relaxed);

      session.send_text("echo: " + message);

      metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);
    });

  ws.start();

  metricsThread.join();

  return 0;
}
```

## Metrics with AttachedRuntime

When HTTP and WebSocket run together, the metrics object can be shared by WebSocket, long-polling, and a dedicated exporter.

```cpp
vix::App app;

auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::WebSocketMetrics metrics;

vix::websocket::Server ws{app.config(), executor};

vix::websocket::LongPollingBridge bridge{
    &metrics,
    std::chrono::seconds{60},
    256};

ws.attach_long_polling_bridge(&bridge);

vix::websocket::AttachedRuntime runtime{app, ws, executor};

app.run(8080);
```

The metrics endpoint can be served separately:

```txt
http://localhost:9100/metrics
```

## Counter vs gauge

Counters only increase.

Examples:

```txt
connections_total
messages_in_total
messages_out_total
errors_total
lp_sessions_total
lp_polls_total
lp_messages_enqueued_total
lp_messages_drained_total
```

Gauges can go up and down.

Examples:

```txt
connections_active
lp_sessions_active
lp_messages_buffered
```

Use the correct behavior when updating metrics.

## Detecting connection leaks

Watch:

```txt
connections_active
```

If active connections never go down after clients disconnect, there may be a cleanup issue.

## Detecting message load

Watch:

```txt
messages_in_total
messages_out_total
```

These counters show how much realtime traffic the server is processing.

## Detecting errors

Watch:

```txt
errors_total
```

A rising error count can indicate:

- invalid clients
- bad frames
- network resets
- oversized messages
- application-level failures
- long-polling misuse

## Detecting long-polling pressure

Watch:

```txt
lp_messages_buffered
```

If this value keeps increasing, HTTP fallback clients may not be draining messages.

Also watch:

```txt
lp_polls_total
lp_messages_enqueued_total
lp_messages_drained_total
```

These values show whether clients are polling often enough.

## Production usage

In production, expose metrics through a port that your monitoring system can scrape.

Example:

```dotenv
WEBSOCKET_METRICS_PORT=9100
```

Then run the exporter with that port:

```cpp
vix::websocket::run_metrics_http_exporter(
    metrics,
    "0.0.0.0",
    9100);
```

You can put the endpoint behind internal networking or reverse proxy rules.

## Security notes

Metrics can reveal operational details.

In production:

- expose metrics only on internal networks
- restrict public access
- avoid leaking sensitive payloads
- keep metrics focused on counts and state
- do not include user message content in metric labels

The current metrics model uses simple counters and gauges, which is safer than high-cardinality labels.

## Best practices

Use one metrics object per WebSocket runtime.

Pass the same metrics object to long-polling components.

Use relaxed atomic updates for simple counters.

Expose metrics on a dedicated internal port.

Monitor active connections and error totals.

Monitor long-polling buffers when fallback is enabled.

Avoid high-cardinality metrics.

Do not store user payloads in metrics.

## Common mistakes

### Creating multiple unrelated metrics objects

Avoid:

```cpp
vix::websocket::WebSocketMetrics wsMetrics;
vix::websocket::WebSocketMetrics lpMetrics;
```

Prefer one shared metrics object:

```cpp
vix::websocket::WebSocketMetrics metrics;
```

### Forgetting to decrement gauges

If you increment:

```cpp
metrics.connections_active.fetch_add(1);
```

also decrement on close:

```cpp
metrics.connections_active.fetch_sub(1);
```

### Exposing metrics publicly

Avoid exposing `/metrics` to the public internet unless protected.

### Using metrics for logs

Metrics should count and measure.

Use logs for detailed events.

### Adding user data to metric names

Avoid metric names or labels that contain user ids, room ids, emails, or payload content.

## Next steps

Continue with:

- [Message store](./message-store.md)
- [Long polling](./long-polling.md)
- [Attached runtime](./attached-runtime.md)
- [Shutdown](./shutdown.md)
- [API Reference](./api-reference.md)
