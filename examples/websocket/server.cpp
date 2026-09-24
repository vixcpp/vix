#include <vix.hpp>

#include <vix/websocket/AttachedRuntime.hpp>
#include <vix/websocket/HttpApi.hpp>
#include <vix/websocket/LongPollingBridge.hpp>

#include <nlohmann/json.hpp>
#include <chrono>
#include <memory>
#include <string>

using namespace vix;

#include <string>
#include <variant>
#include <memory>

static std::string token_to_string(const vix::json::token &t)
{
  // string only
  if (auto s = std::get_if<std::string>(&t.v))
    return *s;
  return {};
}

static std::string kvs_get_string(const vix::json::kvs &obj, const std::string &key)
{
  const auto &f = obj.flat;

  // Expect pairs: key, value, key, value...
  for (std::size_t i = 0; i + 1 < f.size(); i += 2)
  {
    const std::string k = token_to_string(f[i]);
    if (k == key)
      return token_to_string(f[i + 1]);
  }
  return {};
}

int main()
{
  vix::serve_http_and_ws([](auto &app, auto &ws)
                         {
    static vix::websocket::WebSocketMetrics metrics;

    auto bridge = std::make_shared<vix::websocket::LongPollingBridge>(
      &metrics,
      std::chrono::seconds{60},
      256
    );

    ws.attach_long_polling_bridge(bridge);

    app.get("/", [](auto &, auto &res)
    {
      res.json({"message", "Hello form Vix.cpp"});
    });

    app.get("/ws/poll", [&ws](auto &req, auto &res)
    {
      vix::websocket::http::handle_ws_poll(req, res, ws);
    });

    app.post("/ws/send", [&ws](auto &req, auto &res)
    {
      vix::websocket::http::handle_ws_send(req, res, ws);
    });

    app.get("/ws/status", [&ws](auto &, auto &res)
    {
      auto b = ws.long_polling_bridge();
      res.json(json::kv({
       {"bridge_attached", static_cast<bool>(b)},
        {"sessions", b ? static_cast<int>(b->session_count()) : 0}
      }));
    });

  ws.on_typed_message([&ws](auto &, const std::string &type, const vix::json::kvs &payload)
  {
    if (type != "chat.message")
      return;

    const std::string room = kvs_get_string(payload, "room");
    if (!room.empty())
      ws.broadcast_room_json(room, "chat.message", payload);
    else
      ws.broadcast_json("chat.message", payload);
  }); });

  return 0;
}
