# WebRPC Examples

This page is a practical overview of how to use `vix::webrpc`.

WebRPC is transport-agnostic:

- You build and validate JSON-like payloads using `vix::json::token`
- You parse requests into `RpcRequest`
- You register methods in a `Router`
- You handle single calls, notifications, and batch calls with `Dispatcher`

This guide focuses on the public API and common usage patterns.

---

## 1) Parse a request envelope

Use `RpcRequest::parse()` to validate and extract fields from a raw token.

What you get:

- `req.method` as a string
- `req.id` as a token (may be null)
- `req.params` as a token (may be null)
- helper `req.has_id()` to detect request/response vs notification

```cpp
#include <iostream>

#include <vix/webrpc/Request.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

int main()
{
  token raw = obj({
      "id",
      1,
      "method",
      "ping",
      "params",
      obj({
          "msg",
          "hello",
      }),
  });

  auto parsed = RpcRequest::parse(raw);

  if (std::holds_alternative<RpcError>(parsed))
  {
    std::cerr << "Parse error\n";
    return 1;
  }

  const RpcRequest &req = std::get<RpcRequest>(parsed);

  std::cout << "method = " << req.method << "\n";
  std::cout << "has id = " << req.has_id() << "\n";

  if (auto p = req.params_object_ptr())
    std::cout << "msg = " << p->get_string_or("msg", "") << "\n";

  return 0;
}
```

Notes:

- If parsing fails, you get a structured `RpcError` (no exceptions)
- `params_object_ptr()` returns `nullptr` if params is not an object

---

## 2) Register methods and dispatch with Router

`Router` is your method registry. A handler signature is:

- input: `const Context&`
- output: `RpcResult` which is `variant<token, RpcError>`

A handler should:

- Validate params shape
- Read fields safely
- Return either a token (success) or `RpcError` (failure)

```cpp
#include <iostream>

#include <vix/webrpc/Router.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

int main()
{
  Router router;

  router.add("math.add", [](const Context &ctx) -> RpcResult
  {
    const auto *p = ctx.params_object_ptr();
    if (!p)
      return RpcError::invalid_params("params must be object");

    const auto a = p->get_i64_or("a", 0);
    const auto b = p->get_i64_or("b", 0);

    return obj({
        "sum", a + b,
    });
  });

  token req = obj({
      "id",
      42,
      "method",
      "math.add",
      "params",
      obj({
          "a",
          7,
          "b",
          5,
      }),
  });

  RpcResult out = router.dispatch(req);

  if (std::holds_alternative<RpcError>(out))
  {
    std::cerr << "RPC error\n";
    return 1;
  }

  auto res = std::get<token>(out).as_object_ptr();
  std::cout << "sum = " << res->get_i64_or("sum", 0) << "\n";

  return 0;
}
```

Notes:

- `router.dispatch(raw_token)` parses and dispatches a single request object
- If method is not registered, result is `RpcError::method_not_found(...)`

---

## 3) Notifications with Dispatcher

A notification is a call without an id (or with `id = null`).

Notifications are fire-and-forget:

- handler is executed
- no response is returned

`Dispatcher` handles envelope parsing and response wrapping.

```cpp
#include <iostream>

#include <vix/webrpc/Dispatcher.hpp>
#include <vix/webrpc/Router.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

int main()
{
  Router router;

  router.add("log", [](const Context &ctx) -> RpcResult
  {
    const auto *p = ctx.params_object_ptr();
    if (p)
      std::cout << "log: " << p->get_string_or("msg", "") << "\n";
    return token(nullptr);
  });

  Dispatcher d(router);

  token notification = obj({
      "method",
      "log",
      "params",
      obj({
          "msg",
          "fire and forget",
      }),
  });

  auto res = d.handle(notification);
  if (!res.has_value())
    std::cout << "no response (notification)\n";

  return 0;
}
```

Notes:

- `Dispatcher::handle(...)` returns `std::nullopt` for notifications
- This is the same behavior across HTTP, WebSocket, P2P, etc.

---

## 4) Batch requests

A batch is an array of request objects.

Rules:

- Each item with an id produces a response entry
- Notifications produce no response entry
- If all items are notifications, dispatcher returns no response
- Empty batch is invalid and yields an error response

```cpp
#include <iostream>

#include <vix/webrpc/Dispatcher.hpp>
#include <vix/webrpc/Router.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

int main()
{
  Router router;

  router.add("echo", [](const Context &ctx) -> RpcResult
  {
    return ctx.params;
  });

  Dispatcher d(router);

  token batch = array({
      obj({
          "id",
          1,
          "method",
          "echo",
          "params",
          obj({"x", 10}),
      }),
      obj({
          "method",
          "echo",
          "params",
          obj({"y", 20}), // notification
      }),
      obj({
          "id",
          2,
          "method",
          "echo",
          "params",
          obj({"z", 30}),
      }),
  });

  auto res = d.handle(batch);

  if (!res)
  {
    std::cout << "no responses\n";
    return 0;
  }

  auto arr = res->as_array_ptr();
  std::cout << "responses = " << arr->size() << "\n";

  return 0;
}
```

Notes:

- The returned token is an array of response objects
- Notifications are executed but do not produce response entries

---

## 5) When to use Router vs Dispatcher

Use `Router` when:

- You already have a validated `RpcRequest`
- You only need to handle a single request object
- You want full control over envelope and response

Use `Dispatcher` when:

- You want full envelope handling (parse + response wrapping)
- You need notifications support
- You need batch support

In production, most transports will call `Dispatcher`.


