# Notifications (HTTP + WebSocket)

This example shows how to build a minimal real-time notification system in Vix.cpp.

Goal:
- Send notifications from HTTP
- Receive them instantly via WebSocket
- Keep everything beginner-friendly

## 1) Minimal Notification Server

Description:
- HTTP route POST /notify sends a notification
- WebSocket broadcasts it to all connected clients

main.cpp:

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
  vix::serve_http_and_ws([](App& app, auto& ws)
  {
    // Health check
    app.get("/health", [](Request&, Response& res)
    {
      res.json({"ok", true, "service", "notifications"});
    });

    // Send notification via HTTP
    app.post("/notify", [&ws](Request& req, Response& res)
    {
      const auto& j = req.json();

      std::string title = "Notification";
      std::string message = "Empty";

      if (j.is_object())
      {
        if (j.contains("title") && j["title"].is_string())
          title = j["title"].get<std::string>();

        if (j.contains("message") && j["message"].is_string())
          message = j["message"].get<std::string>();
      }

      ws.broadcast_json("notification", {
        "title", title,
        "message", message
      });

      res.json({
        "sent", true,
        "title", title
      });
    });

  });

  return 0;
}
```

## 2) How To Compile & Run

From your project folder:

```bash
vix run main.cpp
```

By default:
- HTTP runs on: http://127.0.0.1:8080
- WebSocket runs on: ws://127.0.0.1:9090

## 3) Test HTTP Endpoint

Send a notification:

```bash
curl -X POST http://127.0.0.1:8080/notify   -H "Content-Type: application/json"   -d '{"title":"System","message":"Server restarted"}'
```

Expected HTTP response:

```json
{
  "sent": true,
  "title": "System"
}
```

## 4) Test WebSocket Client

Using websocat:

```bash
websocat ws://127.0.0.1:9090/
```

Now call the HTTP /notify endpoint again.
You will instantly receive:

```json
{
  "type": "notification",
  "payload": {
    "title": "System",
    "message": "Server restarted"
  }
}
```

## 5) Beginner Mental Model

HTTP:
- Used to trigger actions (create notification)

WebSocket:
- Used to push real-time updates

This separation is common in production systems.

## 6) Extend This Example

You can extend this by:

- Adding user IDs to target specific clients
- Storing notifications in memory or database
- Adding "read" acknowledgements
- Adding authentication middleware

What You Learned

- How to mix HTTP and WebSocket
- How to broadcast typed messages
- How to test using curl + websocat
- How real-time notification systems are structured

