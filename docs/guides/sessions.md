# Sessions

This guide shows how to use session-style authentication in a Vix application.

## What is a session?

```txt
client → sends session id → server → finds session data → route → reads current user → response
```

Sessions are useful for: login state, dashboards, admin panels, browser-based authentication.

## Session vs token authentication

| Style          | Common use                                         |
|----------------|----------------------------------------------------|
| Bearer token   | APIs, mobile apps, and service-to-service calls.   |
| Session cookie | Browser apps, dashboards, and server-rendered apps.|

## Setup

```bash
vix new session-api
cd session-api
```

```cpp
#include <vix.hpp>
#include <vix/validation.hpp>
```

## Define session state

```cpp
struct Session
{
  std::string id;
  int user_id{};
  std::string email;
  bool authenticated{false};
};

struct AppState
{
  std::unordered_map<std::string, Session> sessions;
};
```

## Helper functions

```cpp
// Simple session id (use secure random ids in production)
static std::string make_session_id(int user_id)
{
  return "dev-session-" + std::to_string(user_id);
}

// Read session id from header
static std::string session_id_from_request(Request &req)
{
  return req.header("X-Session-Id");
}

// Find current session
static Session *current_session(AppState &state, Request &req)
{
  const std::string session_id = session_id_from_request(req);

  if (session_id.empty())
    return nullptr;

  auto it = state.sessions.find(session_id);
  if (it == state.sessions.end() || !it->second.authenticated)
    return nullptr;

  return &it->second;
}
```

## Session routes

```cpp
// GET /session — check session state
// POST /session/login — create session
// POST /session/logout — destroy session
// GET /dashboard — protected route
```

## Test the API

```bash
# Health
curl -i http://127.0.0.1:8080/health

# Check session before login
curl -i http://127.0.0.1:8080/session

# Login
curl -i -X POST http://127.0.0.1:8080/session/login \
  -H "Content-Type: application/json" \
  -d '{"email":"ada@example.com","password":"password123"}'

# Check session after login
curl -i http://127.0.0.1:8080/session -H "X-Session-Id: dev-session-1"

# Access dashboard
curl -i http://127.0.0.1:8080/dashboard -H "X-Session-Id: dev-session-1"

# Logout
curl -i -X POST http://127.0.0.1:8080/session/logout -H "X-Session-Id: dev-session-1"
```

## Cookie-based session shape

For browser apps, login sends:

```text
Set-Cookie: session_id=dev-session-1; Path=/; HttpOnly; SameSite=Lax
```

Setting a cookie with Vix:

```cpp
#include <vix/middleware/http/cookies.hpp>

vix::middleware::cookies::Cookie cookie;
cookie.name = "session_id";
cookie.value = session_id;
cookie.path = "/";
cookie.http_only = true;
cookie.secure = false;   // true for HTTPS production
cookie.same_site = "Lax";
cookie.max_age = 3600;

vix::middleware::cookies::set(res, cookie);
```

Reading a cookie:

```cpp
auto value = vix::middleware::cookies::get(req, "session_id");
if (!value) {
  respond_error(res, 401, "missing session");
  return;
}
const std::string session_id = *value;
```

## Session status codes

| Status | Meaning                                      |
|--------|----------------------------------------------|
| `200`  | Session exists or request succeeded.         |
| `400`  | Login request body is invalid.               |
| `401`  | Session is missing, expired, or invalid.     |
| `403`  | Session exists, but access is not allowed.   |

## Production notes

For production sessions, use: random session ids, HTTPS, `HttpOnly` cookies, `Secure` cookies, `SameSite=Lax` or `Strict`, session expiration, persistent session store, CSRF protection for write requests, rate limiting on login, password hashing.

## Common mistakes

### Making session ids predictable

`dev-session-1` is not safe. Use secure random session ids in production.

### Forgetting `HttpOnly`

```cpp
cookie.http_only = true;   // prevents JS from reading the cookie
```

### Forgetting to return after unauthorized

```cpp
// Wrong
if (session == nullptr) {
  respond_error(res, 401, "unauthorized");
}

res.json({"ok", true});

// Correct
if (session == nullptr) {
  respond_error(res, 401, "unauthorized");
  return;
}
```

### Expecting in-memory sessions to survive restart

For production, store sessions in SQLite, MySQL, Redis, or another persistent store.

## What to use next

- [CORS guide](/guides/cors)
- [Rate limiting guide](/guides/rate-limiting)
- [SQLite API guide](/guides/sqlite-api)
