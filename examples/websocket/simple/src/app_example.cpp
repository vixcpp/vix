/**
 *
 *  @file app_example.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#include <vix/websocket.hpp>
#include <iostream>

using vix::websocket::App;
using vix::websocket::Session;

void handle_chat(
    Session &session,
    const std::string &type,
    const vix::json::kvs &payload)
{
  (void)session;

  if (type == "chat.message")
  {
    auto j = vix::websocket::detail::ws_kvs_to_nlohmann(payload);

    std::string user = j.value("user", "anonymous");
    std::string text = j.value("text", "");
    std::string room = j.value("room", "general");

    std::cout << "[chat][" << room << "] " << user << ": " << text << "\n";
  }
}

int main()
{
  App app{"config/config.json"};

  (void)app.ws("/chat", handle_chat);

  app.run_blocking();
  return 0;
}
