# HTTP Middleware Mega Example (Vix.cpp)

This directory contains a **single, large, practical example** demonstrating how to
build HTTP routes and middleware with **Vix.cpp**.

The goal is to show, in one place, how everything fits together:
routing, middleware, security, parsing, cookies, sessions, caching, etc.

---

## Run the example

```bash
vix run examples/http_middleware/mega_middleware_routes.cpp
```

Server starts on:

```
http://127.0.0.1:8080
```

---

## Quick sanity check

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/_routes
```

---

## Cookies example

### Set a cookie

```bash
curl -i http://127.0.0.1:8080/api/cookie/set
```

You should see a `Set-Cookie` header:

```
Set-Cookie: demo=hello; Path=/; Max-Age=3600; HttpOnly; SameSite=Lax
```

### Read the cookie

```bash
curl -i --cookie "demo=hello" http://127.0.0.1:8080/api/cookie/get
```

Response:

```json
{
  "ok": true,
  "cookie_demo": "hello",
  "has_cookie": true
}
```

---

## Session example (signed cookie + server store)

Sessions are handled by the **session middleware**:

- Session id stored in a signed cookie (`sid`)
- Session data stored server-side
- Automatic creation on first request
- Persisted across requests
- Destroyable (logout)

### First request (creates session)

```bash
curl -i http://127.0.0.1:8080/api/session/whoami
```

Response example:

```json
{
  "ok": true,
  "session": true,
  "sid": "e4a1f0...",
  "is_new": true,
  "name": "guest",
  "visits": 1
}
```

### Persist session using cookie jar

```bash
curl -i -c jar.txt http://127.0.0.1:8080/api/session/whoami
curl -i -b jar.txt http://127.0.0.1:8080/api/session/whoami
```

You should see `visits` increase on each request.

---

### Update session data

```bash
curl -i -b jar.txt -X POST http://127.0.0.1:8080/api/session/setname/gaspard
```

Then:

```bash
curl -i -b jar.txt http://127.0.0.1:8080/api/session/whoami
```

Response:

```json
{
  "name": "gaspard",
  "visits": 3
}
```

---

### Logout (destroy session)

```bash
curl -i -b jar.txt -X POST http://127.0.0.1:8080/api/session/logout
```

This will:

- Delete server-side session
- Clear the session cookie (`Max-Age=0`)

Next request creates a new session.

---

## What this example demonstrates

- App routing (`GET`, `POST`, path params)
- Global middleware vs prefix middleware
- Context-based middleware (`adapt_ctx`)
- Legacy HTTP middleware adaptation
- RequestState (typed state storage)
- Cookies (parse + set)
- Sessions (signed cookie + store)
- JSON / Form / Multipart parsing
- API key protection
- RBAC-style guards
- HTTP GET cache
- CSRF, CORS, rate limiting
- Debug and observability patterns

---

## Philosophy

This file is intentionally **big and repetitive**.

It is meant to answer:

> "How do I actually write routes and middleware with Vix.cpp?"

By reading a single file.

If you understand this example, you understand **90% of Vix.cpp HTTP middleware usage**.

