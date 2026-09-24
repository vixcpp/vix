/**
 *
 *  @file examples/websocket/06_metrics_runtime/main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  Metrics runtime example.
 *  Goal:
 *    - run HTTP + WebSocket together
 *    - expose Prometheus-style metrics on /metrics
 *    - demonstrate a small realtime runtime with observability
 *
 */

#include <atomic>
#include <memory>
#include <string>

#include <vix.hpp>
#include <vix/json/build.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
#include <vix/websocket/Metrics.hpp>

namespace
{
  namespace J = vix::json;

  /**
   * @brief Small facade that owns the runtime objects and metrics state.
   */
  struct MetricsRuntime
  {
    vix::config::Config config{".env"};
    std::shared_ptr<vix::executor::RuntimeExecutor> executor{
        std::make_shared<vix::executor::RuntimeExecutor>(1u)};
    vix::App app{executor};
    vix::websocket::Server ws{config, executor};
    vix::websocket::WebSocketMetrics metrics{};

    std::atomic<std::uint64_t> ping_requests{0};
    std::atomic<std::uint64_t> chat_messages{0};
  };

  void register_http_routes(MetricsRuntime &runtime)
  {
    runtime.app.get(
        "/",
        [](vix::Request &, vix::Response &res)
        {
          res.json(J::o(
              "name", "Vix Metrics Runtime",
              "message", "HTTP + WebSocket runtime with metrics",
              "framework", "Vix.cpp"));
        });

    runtime.app.get(
        "/health",
        [](vix::Request &, vix::Response &res)
        {
          res.json(J::o(
              "status", "ok",
              "service", "metrics-runtime"));
        });

    runtime.app.get(
        "/stats",
        [&runtime](vix::Request &, vix::Response &res)
        {
          res.json(J::o(
              "connections_total", runtime.metrics.connections_total.load(std::memory_order_relaxed),
              "connections_active", runtime.metrics.connections_active.load(std::memory_order_relaxed),
              "messages_in_total", runtime.metrics.messages_in_total.load(std::memory_order_relaxed),
              "messages_out_total", runtime.metrics.messages_out_total.load(std::memory_order_relaxed),
              "errors_total", runtime.metrics.errors_total.load(std::memory_order_relaxed),
              "ping_requests", runtime.ping_requests.load(std::memory_order_relaxed),
              "chat_messages", runtime.chat_messages.load(std::memory_order_relaxed)));
        });

    runtime.app.get(
        "/features",
        [](vix::Request &, vix::Response &res)
        {
          res.json(J::o(
              "http", true,
              "websocket", true,
              "metrics", true,
              "events", J::a("app.ping", "chat.message", "system.metrics")));
        });

    runtime.app.get(
        "/metrics",
        [&runtime](vix::Request &, vix::Response &res)
        {
          res.text(runtime.metrics.render_prometheus());
        });
  }

  void register_ws_lifecycle(MetricsRuntime &runtime)
  {
    runtime.ws.on_open(
        [&runtime](vix::websocket::Session &session)
        {
          (void)session;

          runtime.metrics.connections_total.fetch_add(1, std::memory_order_relaxed);
          runtime.metrics.connections_active.fetch_add(1, std::memory_order_relaxed);
          runtime.metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);

          runtime.ws.broadcast_json(
              "system.connected",
              {
                  "message",
                  "A WebSocket client connected",
              });
        });

    runtime.ws.on_close(
        [&runtime](vix::websocket::Session &session)
        {
          (void)session;

          runtime.metrics.connections_active.fetch_sub(1, std::memory_order_relaxed);
        });

    runtime.ws.on_error(
        [&runtime](vix::websocket::Session &session, const std::string &error)
        {
          (void)session;
          (void)error;

          runtime.metrics.errors_total.fetch_add(1, std::memory_order_relaxed);
        });
  }

  void register_ws_protocol(MetricsRuntime &runtime)
  {
    runtime.ws.on_typed_message(
        [&runtime](
            vix::websocket::Session &session,
            const std::string &type,
            const vix::json::kvs &payload)
        {
          (void)session;
          (void)payload;

          runtime.metrics.messages_in_total.fetch_add(1, std::memory_order_relaxed);

          if (type == "app.ping")
          {
            runtime.ping_requests.fetch_add(1, std::memory_order_relaxed);
            runtime.metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);

            runtime.ws.broadcast_json(
                "app.pong",
                {
                    "status",
                    "ok",
                    "transport",
                    "websocket",
                });

            return;
          }

          if (type == "chat.message")
          {
            runtime.chat_messages.fetch_add(1, std::memory_order_relaxed);
            runtime.metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);

            runtime.ws.broadcast_json("chat.message", payload);
            return;
          }

          if (type == "system.metrics")
          {
            runtime.metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);

            runtime.ws.broadcast_json(
                "system.metrics.snapshot",
                {
                    "connections_total",
                    static_cast<long long>(
                        runtime.metrics.connections_total.load(std::memory_order_relaxed)),
                    "connections_active",
                    static_cast<long long>(
                        runtime.metrics.connections_active.load(std::memory_order_relaxed)),
                    "messages_in_total",
                    static_cast<long long>(
                        runtime.metrics.messages_in_total.load(std::memory_order_relaxed)),
                    "messages_out_total",
                    static_cast<long long>(
                        runtime.metrics.messages_out_total.load(std::memory_order_relaxed)),
                    "errors_total",
                    static_cast<long long>(
                        runtime.metrics.errors_total.load(std::memory_order_relaxed)),
                });

            return;
          }

          runtime.metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);

          runtime.ws.broadcast_json(
              "app.unknown",
              {
                  "type",
                  type,
                  "message",
                  "Unknown event type",
              });
        });
  }

  void configure(MetricsRuntime &runtime)
  {
    register_http_routes(runtime);
    register_ws_lifecycle(runtime);
    register_ws_protocol(runtime);
  }

  void run(MetricsRuntime &runtime)
  {
    const int http_port = runtime.config.getServerPort();
    vix::run_http_and_ws(runtime.app, runtime.ws, runtime.executor, http_port);
  }
} // namespace

int main()
{
  MetricsRuntime runtime;
  configure(runtime);
  run(runtime);
  return 0;
}
