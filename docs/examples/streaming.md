
# Streaming (HTTP + WebSocket)

This guide explains streaming patterns in Vix.cpp.

Streaming means sending data progressively instead of sending one single response.

You will learn:

- HTTP chunk-style streaming
- Server-Sent style behavior (SSE-like pattern)
- WebSocket streaming
- How to test everything step by step

1) HTTP Streaming (Chunked Response Style)

Description:
Send multiple chunks over one HTTP request.

main.cpp:

```cpp
#include <vix.hpp>
#include <chrono>
#include <thread>

using namespace vix;

int main()
{
  App app;

  app.get("/stream", [](Request&, Response& res)
  {
    res.set_header("Content-Type", "text/plain");

    for (int i = 1; i <= 5; ++i)
    {
      res.write("Chunk " + std::to_string(i) + "\n");
      res.flush();
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    res.end();
  });

  app.run(8080);
}
```

How to test:

Terminal 1:
```bash
    vix run main.cpp
```
Terminal 2:
```bash
    curl http://127.0.0.1:8080/stream
```

You should see chunks appearing progressively.

2) Server-Sent Events (SSE-like Pattern)

Description:
Push events over HTTP continuously.

main.cpp:

```cpp
#include <vix.hpp>
#include <chrono>
#include <thread>

using namespace vix;

int main()
{
  App app;

  app.get("/events", [](Request&, Response& res)
  {
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");

    for (int i = 0; i < 5; ++i)
    {
      res.write("data: event " + std::to_string(i) + "\n\n");
      res.flush();
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    res.end();
  });

  app.run(8080);
}
```

Test:
```bash
    curl http://127.0.0.1:8080/events
```
3) WebSocket Streaming (Live Broadcast)

Description:
Push real-time data continuously to connected clients.

main.cpp:

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
#include <chrono>
#include <thread>

using namespace vix;

int main()
{
  vix::serve_http_and_ws([](App& app, auto& ws)
  {
    app.get("/", [](Request&, Response& res)
    {
      res.send("WebSocket streaming server running");
    });

    ws.on_open([&ws](auto&)
    {
      std::thread([&ws]()
      {
        for (int i = 0; i < 5; ++i)
        {
          ws.broadcast_json("stream.tick", {
            "value", i
          });

          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      }).detach();
    });
  });

  return 0;
}
```

How to test:

Terminal 1:
```bash
    vix run main.cpp
```
Terminal 2:
```bash
    websocat ws://127.0.0.1:9090/
```

You should receive JSON messages every second.

4) When to Use Each Streaming Type

HTTP chunk streaming:
- Large file generation
- Report generation
- Progressive output

SSE-like streaming:
- Live logs
- Monitoring dashboards
- One-direction event push

WebSocket streaming:
- Chat
- Multiplayer state sync
- Real-time notifications
- Presence systems

Key Takeaways

- Streaming avoids blocking until full data is ready.
- WebSocket is best for bi-directional live systems.
- HTTP streaming works well for simple progressive output.
- Always test using curl or websocat first.

