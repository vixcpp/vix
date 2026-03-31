# Streaming

This page explains streaming in Vix Inference.

Streaming is used when the model produces tokens progressively. Instead
of waiting for the full response, the server sends chunks as they
arrive.

------------------------------------------------------------------------

## When to use streaming

Use streaming when you need:

-   low latency first token
-   live UI updates (chat typing effect)
-   long generations (summaries, code, reasoning)
-   server push without buffering full payload

------------------------------------------------------------------------

## Core idea

A streaming response is a sequence of chunks:

1)  client sends a request
2)  server starts inference
3)  server emits chunks
4)  server ends the stream

The client processes chunks in order.

------------------------------------------------------------------------

## Minimal HTTP example

This example shows the intended shape. Exact helper names may differ
based on your inference module version.

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.post("/infer/stream", [](Request& req, Response& res) {
    (void)req;

    // 1) Set streaming headers (SSE style is common)
    res.header("content-type", "text/event-stream");
    res.header("cache-control", "no-cache");
    res.header("connection", "keep-alive");

    // 2) Start streaming
    // send a first chunk
    res.write("event: start\n");
    res.write("data: ok\n\n");

    // 3) Emit chunks (fake loop here)
    for (int i = 0; i < 5; ++i)
    {
      res.write("event: token\n");
      res.write("data: hello\n\n");
      res.flush();
    }

    // 4) End
    res.write("event: end\n");
    res.write("data: done\n\n");
    res.flush();
    res.end();
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## Recommended chunk format

Use a typed envelope. This stays consistent with WebSocket typed
messages.

Example chunk event:

``` json
{
  "type": "inference.token",
  "payload": {
    "text": "hello"
  }
}
```

End event:

``` json
{
  "type": "inference.end",
  "payload": {
    "reason": "stop"
  }
}
```

------------------------------------------------------------------------

## Backpressure

Streaming must handle slow clients.

Recommended rules:

-   flush only when you have new data
-   avoid huge chunks
-   cap total bytes per request
-   enforce max duration per stream
-   close the stream when the client disconnects

------------------------------------------------------------------------

## Error handling

Errors should be streamed as an event, then the stream must end.

Example:

``` json
{
  "type": "inference.error",
  "payload": {
    "message": "provider timeout"
  }
}
```

Then send `inference.end`.

------------------------------------------------------------------------

## Notes

-   Streaming is transport specific: HTTP streaming (SSE, chunked) and
    WebSocket streaming are both valid.
-   Keep chunks small and predictable.
-   Always send an explicit end event.

Next page: providers and routing.

