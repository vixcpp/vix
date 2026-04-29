/**
 *
 *  @file examples/websocket/08_realtime_dashboard/main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  Realtime dashboard example.
 *  Goal:
 *    - serve a small dashboard UI over HTTP
 *    - push live status updates over WebSocket
 *    - expose health, stats, and metrics endpoints
 *
 */

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <vix.hpp>
#include <vix/json/build.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
#include <vix/websocket/Metrics.hpp>

namespace
{
  namespace J = vix::json;

  struct DashboardSnapshot
  {
    std::string service{"realtime-dashboard"};
    std::string status{"healthy"};
    std::string message{"System operational"};
    std::int64_t uptime_seconds{0};
    std::int64_t tick{0};
    std::int64_t active_clients{0};
  };

  class DashboardState
  {
  public:
    void set_status(std::string status, std::string message)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      snapshot_.status = std::move(status);
      snapshot_.message = std::move(message);
    }

    void set_active_clients(std::int64_t count)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      snapshot_.active_clients = count;
    }

    void update_uptime(std::int64_t uptime_seconds)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      snapshot_.uptime_seconds = uptime_seconds;
    }

    void increment_tick()
    {
      std::lock_guard<std::mutex> lock(mutex_);
      ++snapshot_.tick;
    }

    [[nodiscard]] DashboardSnapshot snapshot() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return snapshot_;
    }

  private:
    mutable std::mutex mutex_;
    DashboardSnapshot snapshot_{};
  };

  struct DashboardRuntime
  {
    vix::config::Config config{".env"};
    std::shared_ptr<vix::executor::RuntimeExecutor> executor{
        std::make_shared<vix::executor::RuntimeExecutor>(1u)};
    vix::App app{executor};
    vix::websocket::Server ws{config, executor};
    std::shared_ptr<DashboardState> state{std::make_shared<DashboardState>()};
    vix::websocket::WebSocketMetrics metrics{};

    std::atomic<bool> publisher_running{true};
    std::thread publisher_thread{};
    std::chrono::steady_clock::time_point started_at{
        std::chrono::steady_clock::now()};
  };

  [[nodiscard]] J::OrderedJson snapshot_json(const DashboardSnapshot &s)
  {
    return J::o(
        "service", s.service,
        "status", s.status,
        "message", s.message,
        "uptime_seconds", s.uptime_seconds,
        "tick", s.tick,
        "active_clients", s.active_clients);
  }

  void register_asset_routes(DashboardRuntime &runtime)
  {
    const std::filesystem::path root =
        "examples/websocket/08_realtime_dashboard/public";

    runtime.app.get(
        "/",
        [root](vix::Request &, vix::Response &res)
        {
          res.file(root / "index.html");
        });

    runtime.app.get(
        "/app.js",
        [root](vix::Request &, vix::Response &res)
        {
          res.file(root / "app.js");
        });

    runtime.app.get(
        "/app.css",
        [root](vix::Request &, vix::Response &res)
        {
          res.file(root / "app.css");
        });
  }

  void register_http_routes(DashboardRuntime &runtime)
  {
    runtime.app.get(
        "/health",
        [state = runtime.state](vix::Request &, vix::Response &res)
        {
          const auto snapshot = state->snapshot();

          res.json(J::o(
              "ok", snapshot.status == "healthy",
              "service", snapshot.service,
              "status", snapshot.status,
              "message", snapshot.message));
        });

    runtime.app.get(
        "/stats",
        [state = runtime.state](vix::Request &, vix::Response &res)
        {
          res.json(snapshot_json(state->snapshot()));
        });

    runtime.app.get(
        "/metrics",
        [&runtime](vix::Request &, vix::Response &res)
        {
          res.text(runtime.metrics.render_prometheus());
        });

    runtime.app.get(
        "/api/features",
        [](vix::Request &, vix::Response &res)
        {
          res.json(J::o(
              "http", true,
              "websocket", true,
              "dashboard", true,
              "events", J::a("dashboard.snapshot", "dashboard.refresh", "dashboard.status")));
        });
  }

  void publish_snapshot(DashboardRuntime &runtime)
  {
    const auto now = std::chrono::steady_clock::now();
    const auto uptime =
        std::chrono::duration_cast<std::chrono::seconds>(now - runtime.started_at).count();

    runtime.state->update_uptime(static_cast<std::int64_t>(uptime));
    runtime.state->increment_tick();

    const auto snapshot = runtime.state->snapshot();

    runtime.metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);

    runtime.ws.broadcast_json(
        "dashboard.snapshot",
        {
            "service",
            snapshot.service,
            "status",
            snapshot.status,
            "message",
            snapshot.message,
            "uptime_seconds",
            snapshot.uptime_seconds,
            "tick",
            snapshot.tick,
            "active_clients",
            snapshot.active_clients,
        });
  }

  void start_background_publisher(DashboardRuntime &runtime)
  {
    runtime.publisher_thread = std::thread(
        [&runtime]()
        {
          while (runtime.publisher_running.load(std::memory_order_acquire))
          {
            publish_snapshot(runtime);
            std::this_thread::sleep_for(std::chrono::seconds{2});
          }
        });
  }

  void stop_background_publisher(DashboardRuntime &runtime)
  {
    const bool was_running =
        runtime.publisher_running.exchange(false, std::memory_order_acq_rel);

    if (!was_running)
    {
      return;
    }

    if (runtime.publisher_thread.joinable())
    {
      runtime.publisher_thread.join();
    }
  }

  void register_ws_lifecycle(DashboardRuntime &runtime)
  {
    runtime.ws.on_open(
        [&runtime](vix::websocket::Session &session)
        {
          (void)session;

          runtime.metrics.connections_total.fetch_add(1, std::memory_order_relaxed);
          runtime.metrics.connections_active.fetch_add(1, std::memory_order_relaxed);

          runtime.state->set_active_clients(
              static_cast<std::int64_t>(
                  runtime.metrics.connections_active.load(std::memory_order_relaxed)));

          runtime.metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);

          const auto snapshot = runtime.state->snapshot();

          runtime.ws.broadcast_json(
              "dashboard.status",
              {
                  "status",
                  snapshot.status,
                  "message",
                  "A dashboard client connected",
                  "active_clients",
                  snapshot.active_clients,
              });
        });

    runtime.ws.on_close(
        [&runtime](vix::websocket::Session &session)
        {
          (void)session;

          runtime.metrics.connections_active.fetch_sub(1, std::memory_order_relaxed);

          runtime.state->set_active_clients(
              static_cast<std::int64_t>(
                  runtime.metrics.connections_active.load(std::memory_order_relaxed)));
        });

    runtime.ws.on_error(
        [&runtime](vix::websocket::Session &session, const std::string &error)
        {
          (void)session;
          (void)error;

          runtime.metrics.errors_total.fetch_add(1, std::memory_order_relaxed);
        });
  }

  void register_ws_protocol(DashboardRuntime &runtime)
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

          if (type == "dashboard.refresh")
          {
            publish_snapshot(runtime);
            return;
          }

          if (type == "dashboard.status")
          {
            runtime.state->set_status("healthy", "Manual status check requested");
            publish_snapshot(runtime);
            return;
          }

          runtime.metrics.messages_out_total.fetch_add(1, std::memory_order_relaxed);

          runtime.ws.broadcast_json(
              "dashboard.unknown",
              {
                  "type",
                  type,
                  "message",
                  "Unknown dashboard event type",
              });
        });
  }

  void configure(DashboardRuntime &runtime)
  {
    register_asset_routes(runtime);
    register_http_routes(runtime);
    register_ws_lifecycle(runtime);
    register_ws_protocol(runtime);
    start_background_publisher(runtime);
  }

  void run(DashboardRuntime &runtime)
  {
    const int http_port = runtime.config.getServerPort();

    vix::register_ws_openapi_docs_once();

    if (auto r = runtime.app.router())
    {
      vix::openapi::register_openapi_and_docs(*r, "Vix API", "1.33.0");
    }

    vix::websocket::AttachedRuntime attached{
        runtime.app,
        runtime.ws,
        runtime.executor};

    runtime.app.set_shutdown_callback(
        [&runtime, &attached]()
        {
          stop_background_publisher(runtime);
          attached.request_stop();
        });

    runtime.app.listen(
        http_port,
        [&]()
        {
          if (!runtime.app.has_server_ready_info())
          {
            vix::console.warn("server ready info not available");
            return;
          }

          auto info = runtime.app.server_ready_info();
          info.show_ws = true;
          info.ws_scheme = "ws";
          info.ws_host = "localhost";
          info.ws_port = runtime.ws.port();
          info.ws_path = "/";

          vix::utils::RuntimeBanner::emit_server_ready(info);
        });

    runtime.app.wait();
    runtime.app.close();

    stop_background_publisher(runtime);
    attached.finalize_shutdown();
  }
} // namespace

int main()
{
  DashboardRuntime runtime;
  configure(runtime);
  run(runtime);
  stop_background_publisher(runtime);
  return 0;
}
