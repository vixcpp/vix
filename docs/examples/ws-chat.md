# WebSocket Chat Example

This example shows how to build a minimal real-time chat server using
Vix.cpp.

It is designed for:

-   C++ beginners learning WebSocket
-   Backend developers building real-time systems
-   Production-ready typed message handling

## 1. Goal

We will build:

-   One HTTP route: `/health`
-   One WebSocket endpoint
-   A simple typed protocol: { "type": "chat.message", "payload": { ...
    } }
-   Broadcast messages to all connected clients

## 2. Minimal Chat Server (Single File)

Create:

main.cpp

``` cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
  vix::serve_http_and_ws([](App& app, auto& ws)
  {
    // HTTP route
    app.get("/health", [](Request&, Response& res)
    {
      res.json({
        "ok", true,
        "service", "chat"
      });
    });

    // WebSocket events

    ws.on_open([&ws](auto&)
    {
      ws.broadcast_json("chat.system", {
        "text", "A user connected"
      });
    });

    ws.on_close([&ws](auto&)
    {
      ws.broadcast_json("chat.system", {
        "text", "A user disconnected"
      });
    });

    ws.on_typed_message(
      [&ws](auto&, const std::string& type,
            const vix::json::kvs& payload)
      {
        if (type == "chat.message")
        {
          ws.broadcast_json("chat.message", payload);
          return;
        }

        ws.broadcast_json("chat.unknown", {
          "type", type
        });
      });
  });

  return 0;
}
```

## 3. How To Run

From your project directory:

``` bash
vix run main.cpp
```

You should see the server listening on port 8080.

## 4. Test HTTP Route

``` bash
curl http://127.0.0.1:8080/health
```

Expected output:

``` json
{
  "ok": true,
  "service": "chat"
}
```

## 5. Test WebSocket (Beginner Method)

Install websocat if needed:

Linux:

``` bash
sudo apt install websocat
```

Then connect:

``` bash
websocat ws://127.0.0.1:8080/
```

Now send:

``` json
{"type":"chat.message","payload":{"user":"Alice","text":"Hello!"}}
```

Open two terminals running websocat to see broadcast working.

## 6. Message Protocol

Every WebSocket message follows this structure:

``` json
{
  "type": "chat.message",
  "payload": {
    "user": "Alice",
    "text": "Hello"
  }
}
```

Rules:

-   type defines the event
-   payload contains the data
-   Server decides how to route based on type

## 7. Recommended Structure For Real Projects

Instead of a single file:

src/ main.cpp chat/ ChatHandler.hpp ChatHandler.cpp

Keep WebSocket logic separate from HTTP routes.

## 8. Common Extensions

You can extend this example with:

-   Authentication before joining chat
-   Rooms (ws.join(room))
-   Private messaging
-   Presence tracking
-   Database persistence
-   Rate limiting
-   Message validation

## 9. Key Concepts Learned

-   How to start HTTP + WebSocket in Vix
-   How to broadcast messages
-   How typed protocols work
-   How to test real-time systems quickly
-   How to structure C++ WebSocket backends

This example is intentionally minimal but production-ready in structure.

