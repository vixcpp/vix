# WebRPC Guide

This guide explains how to use `vix::webrpc` to build a small, explicit RPC layer.

WebRPC is transport-agnostic. It does not care if your payload comes from HTTP, WebSocket, P2P, or CLI.
Your transport layer should only translate bytes to `vix::json::token`, call WebRPC, then translate the response back to bytes.

---

## What you build with WebRPC

A typical setup has three layers:

1) **Envelope**: `RpcRequest` and `RpcResponse` define what a call looks like.
2) **Methods**: `Router` registers method handlers and executes them.
3) **Orchestration**: `Dispatcher` adds notification and batch support, and produces response envelopes.

Inside handlers you get a **Context** which is a read-only view of the request.

---

## Request shape

A request is a JSON object token:

- `method` (required, non-empty string)
- `id` (optional; null or missing means notification)
- `params` (optional; any JSON-like token)

Example:

```json
{ "id": 1, "method": "ping", "params": { "msg": "hello" } }
```

---

## Step 1: Parse a request

When you want to validate a raw token and inspect fields, use `RpcRequest::parse()`.

Key points:

- Parsing returns a value, not exceptions.
- On failure you get a structured `RpcError`.
- Use `has_id()` to detect request vs notification.

Minimal pattern:

```cpp
auto parsed = RpcRequest::parse(raw);
if (std::holds_alternative<RpcError>(parsed)) {
  // handle parse error
}
RpcRequest req = std::get<RpcRequest>(parsed);
```

For a complete runnable example, see the examples file below.

---

## Step 2: Register methods with Router

A WebRPC method is a handler:

- Input: `const Context&`
- Output: `RpcResult` which is `variant<token, RpcError>`

Guidelines for a good handler:

- Validate `params` shape first (object vs array vs null).
- Read fields using safe helpers like `get_i64_or`, `get_string_or`.
- Return `RpcError::invalid_params(...)` when input does not match your contract.

Minimal pattern:

```cpp
router.add("math.add", [](const Context& ctx) -> RpcResult {
  const auto* p = ctx.params_object_ptr();
  if (!p) return RpcError::invalid_params("params must be object");
  return vix::json::obj({ "sum", p->get_i64_or("a", 0) + p->get_i64_or("b", 0) });
});
```

Router can dispatch a single raw request object:

```cpp
RpcResult out = router.dispatch(raw_request_token);
```

---

## Step 3: Use Dispatcher for real transports

`Dispatcher` is the recommended entry point for HTTP, WebSocket, or P2P integrations.

It handles:

- Parsing envelopes
- Notifications (no id -> no response)
- Batch requests (array of request objects)
- Wrapping results into `RpcResponse` JSON

You call:

```cpp
Dispatcher d(router);
auto response = d.handle(payload_token, "http", &meta);
```

Return value:

- `std::nullopt` when no response should be sent (notification only)
- Otherwise a `token` representing a response object or an array of responses

---

## Notifications

A notification is a call without `id` (or `id = null`).

Rules:

- Handler runs.
- No response is produced.

Use notifications for fire-and-forget operations like logs, telemetry, presence.

---

## Batch requests

A batch is an array of request objects.

Rules:

- Each item with an id yields a response entry.
- Notifications yield no response entry.
- If every item is a notification, the dispatcher returns no response.
- Empty batch is invalid and yields an error response with `id = null`.

Batch is useful when latency is high and you want fewer round-trips.

---

## What goes in transport code

Keep the transport adapter thin:

1) Receive bytes
2) Parse into `vix::json::token`
3) Call `Dispatcher::handle(payload, transport, meta)`
4) If a response exists, serialize it and send it back

Business logic lives in handlers, not in the transport.

