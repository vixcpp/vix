/**
 *
 *  @file examples/websocket/07_client_runtime/main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  Native WebSocket client example.
 *  Goal:
 *    - connect to a Vix WebSocket server
 *    - register lifecycle callbacks
 *    - enable heartbeat and auto-reconnect
 *    - send typed JSON messages
 *
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <vix/json/build.hpp>
#include <vix/websocket.hpp>
#include <vix/utils/Env.hpp>

namespace
{
  namespace J = vix::json;

  /**
   * @brief Runtime settings for the client example.
   */
  struct ClientSettings
  {
    std::string host{"127.0.0.1"};
    std::string port{"9090"};
    std::string target{"/"};
    std::string user{"client-demo"};
    std::string room{"general"};
  };

  /**
   * @brief Small facade that owns the client instance and configuration.
   */
  class ClientRuntime
  {
  public:
    explicit ClientRuntime(ClientSettings settings)
        : settings_(std::move(settings)),
          client_(vix::websocket::Client::create(
              settings_.host,
              settings_.port,
              settings_.target))
    {
    }

    void configure()
    {
      register_lifecycle_callbacks();
      configure_runtime_behavior();
    }

    void run()
    {
      client_->connect();

      wait_until_connected();

      if (!client_->is_connected())
      {
        std::cerr << "[client] failed to connect\n";
        return;
      }

      send_join();
      send_ping();
      send_demo_message();

      std::this_thread::sleep_for(std::chrono::seconds{5});

      send_typing();
      std::this_thread::sleep_for(std::chrono::seconds{2});

      send_leave();
      std::this_thread::sleep_for(std::chrono::seconds{1});

      client_->close();
    }

  private:
    void register_lifecycle_callbacks()
    {
      client_->on_open(
          [this]()
          {
            std::cout
                << "[client] connected to ws://"
                << settings_.host << ":"
                << settings_.port
                << settings_.target << "\n";
          });

      client_->on_message(
          [](const std::string &message)
          {
            std::cout << "[client] received: " << message << "\n";
          });

      client_->on_close(
          []()
          {
            std::cout << "[client] connection closed\n";
          });

      client_->on_error(
          [](const std::string &error)
          {
            std::cerr << "[client] error: " << error << "\n";
          });
    }

    void configure_runtime_behavior()
    {
      client_->enable_auto_reconnect(true, std::chrono::seconds{3});
      client_->enable_heartbeat(std::chrono::seconds{15});
    }

    void wait_until_connected()
    {
      constexpr int max_attempts = 50;

      for (int i = 0; i < max_attempts; ++i)
      {
        if (client_->is_connected())
        {
          return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{100});
      }
    }

    void send_join()
    {
      client_->send(
          "chat.join",
          {
              "user",
              settings_.user,
              "room",
              settings_.room,
          });

      std::cout << "[client] sent chat.join\n";
    }

    void send_ping()
    {
      client_->send(
          "app.ping",
          {
              "user",
              settings_.user,
          });

      std::cout << "[client] sent app.ping\n";
    }

    void send_demo_message()
    {
      client_->send(
          "chat.message",
          {
              "user",
              settings_.user,
              "room",
              settings_.room,
              "text",
              "Hello from the native Vix client",
              "meta",
              vix::json::obj({
                  "kind",
                  "demo",
                  "source",
                  "07_client_runtime",
              }),
          });

      std::cout << "[client] sent chat.message\n";
    }

    void send_typing()
    {
      client_->send(
          "chat.typing",
          {
              "user",
              settings_.user,
              "room",
              settings_.room,
          });

      std::cout << "[client] sent chat.typing\n";
    }

    void send_leave()
    {
      client_->send(
          "chat.leave",
          {
              "user",
              settings_.user,
              "room",
              settings_.room,
          });

      std::cout << "[client] sent chat.leave\n";
    }

  private:
    ClientSettings settings_;
    std::shared_ptr<vix::websocket::Client> client_;
  };

  [[nodiscard]] ClientSettings make_settings_from_env()
  {
    ClientSettings settings{};

    if (const char *v = vix::utils::vix_getenv("VIX_WS_HOST"); v && *v)
    {
      settings.host = v;
    }

    if (const char *v = vix::utils::vix_getenv("VIX_WS_PORT"); v && *v)
    {
      settings.port = v;
    }

    if (const char *v = vix::utils::vix_getenv("VIX_WS_TARGET"); v && *v)
    {
      settings.target = v;
    }

    if (const char *v = vix::utils::vix_getenv("VIX_WS_USER"); v && *v)
    {
      settings.user = v;
    }

    if (const char *v = vix::utils::vix_getenv("VIX_WS_ROOM"); v && *v)
    {
      settings.room = v;
    }

    return settings;
  }
} // namespace

int main()
{
  ClientRuntime runtime{make_settings_from_env()};
  runtime.configure();
  runtime.run();
  return 0;
}
