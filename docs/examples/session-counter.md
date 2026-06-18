# Session Counter

This example shows how to use Vix session middleware to store server-side state per browser.

A session is useful when you need to keep small server-managed state across requests, such as:

```txt id="e4c89m"
login state
cart state
flash messages
user preferences
temporary form state
per-browser counters
```

This example builds a small counter.

Each browser gets a session cookie.

The server stores the counter in the session.

## Cookies vs sessions

A cookie stores data in the browser.

A session stores data on the server and usually keeps only a session id in the browser.

Cookie example:

```txt id="j6wzrw"
theme=dark
```

Session example:

```txt id="zm5d8u"
sid=abc123
```

The browser sends `sid`.

The server uses `sid` to load session data.

In this example, the session stores:

```txt id="n0vz5p"
n=1
n=2
n=3
```

The browser only receives the session id cookie.

## What this example builds

The app exposes:

```txt id="d36h7k"
GET /session
GET /session/value
GET /session/reset
GET /session/destroy
```

The behavior is:

```txt id="vv2dzd"
GET /session
  increments the counter

GET /session/value
  reads the current counter

GET /session/reset
  sets the counter back to zero

GET /session/destroy
  destroys the session
```

## Header

Use:

```cpp id="z5m8ww"
#include <vix/middleware.hpp>
```

The session types live in:

```cpp id="w2oiid"
vix::middleware::auth
```

The main state type is:

```cpp id="nhen8j"
vix::middleware::auth::Session
```

## Project structure

Create:

```txt id="mrtncb"
session_counter_demo/
└── session_counter.cpp
```

Create the file:

```bash id="nptbxf"
mkdir session_counter_demo
cd session_counter_demo
touch session_counter.cpp
```

## Source

Open:

```txt id="gswvj9"
session_counter.cpp
```

Add:

```cpp id="tal0gm"
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static int read_counter(const middleware::auth::Session &session)
{
  auto value = session.get("n");

  if (!value)
    return 0;

  try
  {
    return std::stoi(*value);
  }
  catch (...)
  {
    return 0;
  }
}

static void install_middleware(App &app)
{
  app.use("/session", middleware::app::request_id_dev());
  app.use("/session", middleware::app::timing_dev());
  app.use("/session", middleware::app::security_headers_dev());

  middleware::auth::SessionOptions session_options;

  session_options.secret = "dev_session_secret";
  session_options.cookie_name = "sid";
  session_options.cookie_path = "/";
  session_options.secure = false;
  session_options.http_only = true;
  session_options.same_site = "Lax";
  session_options.auto_create = true;

  app.use("/session", middleware::app::adapt_ctx(
    middleware::auth::session(session_options)
  ));
}

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.send(
      "Session counter example\n"
      "\n"
      "Try:\n"
      "  curl -i -c cookies.txt http://127.0.0.1:8080/session\n"
      "  curl -i -b cookies.txt -c cookies.txt http://127.0.0.1:8080/session\n"
      "  curl -i -b cookies.txt http://127.0.0.1:8080/session/value\n"
      "  curl -i -b cookies.txt -c cookies.txt http://127.0.0.1:8080/session/reset\n"
      "  curl -i -b cookies.txt -c cookies.txt http://127.0.0.1:8080/session/destroy\n"
    );
  });

  app.get("/session", [](Request &req, Response &res)
  {
    auto &session =
      req.state<middleware::auth::Session>();

    int n = read_counter(session);
    ++n;

    session.set("n", std::to_string(n));

    res.json({
      "ok", true,
      "counter", n,
      "session_id", session.id,
      "is_new", session.is_new
    });
  });

  app.get("/session/value", [](Request &req, Response &res)
  {
    auto &session =
      req.state<middleware::auth::Session>();

    const int n = read_counter(session);

    res.json({
      "ok", true,
      "counter", n,
      "session_id", session.id,
      "is_new", session.is_new
    });
  });

  app.get("/session/reset", [](Request &req, Response &res)
  {
    auto &session =
      req.state<middleware::auth::Session>();

    session.set("n", "0");

    res.json({
      "ok", true,
      "counter", 0,
      "message", "counter reset"
    });
  });

  app.get("/session/destroy", [](Request &req, Response &res)
  {
    auto &session =
      req.state<middleware::auth::Session>();

    session.destroy();

    res.json({
      "ok", true,
      "message", "session destroyed"
    });
  });
}

int main()
{
  App app;

  install_middleware(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

## Run it

Run:

```bash id="e0ehyy"
vix run session_counter.cpp
```

The server listens on:

```txt id="dsm0mq"
http://127.0.0.1:8080
```

## First request

Use `-c` to save the session cookie:

```bash id="bty6p2"
curl -i \
  -c cookies.txt \
  http://127.0.0.1:8080/session
```

Expected body shape:

```json id="guw5eq"
{
  "ok": true,
  "counter": 1,
  "session_id": "...",
  "is_new": true
}
```

Expected response header shape:

```txt id="qh7j7c"
Set-Cookie: sid=...; Path=/; HttpOnly; SameSite=Lax
```

The server created a session and stored:

```txt id="u9tx6v"
n=1
```

## Second request

Use `-b` to send the saved cookie and `-c` to update it if needed:

```bash id="zzbx0f"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/session
```

Expected body shape:

```json id="tpntlf"
{
  "ok": true,
  "counter": 2,
  "session_id": "...",
  "is_new": false
}
```

The browser sent the same `sid`.

The server loaded the same session.

Then it incremented the counter.

## Read without incrementing

```bash id="gjnyu7"
curl -i \
  -b cookies.txt \
  http://127.0.0.1:8080/session/value
```

Expected body shape:

```json id="alss0p"
{
  "ok": true,
  "counter": 2,
  "session_id": "...",
  "is_new": false
}
```

This route reads the current value without changing it.

## Reset the counter

```bash id="fdd89c"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/session/reset
```

Expected body:

```json id="sz01ie"
{
  "ok": true,
  "counter": 0,
  "message": "counter reset"
}
```

Read again:

```bash id="b41st1"
curl -i \
  -b cookies.txt \
  http://127.0.0.1:8080/session/value
```

Expected counter:

```txt id="trhy0j"
0
```

## Destroy the session

```bash id="hqv483"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/session/destroy
```

Expected body:

```json id="bsx3gb"
{
  "ok": true,
  "message": "session destroyed"
}
```

After destroying, request `/session` again:

```bash id="t3k7li"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/session
```

Expected behavior:

```txt id="fyi9nl"
a new session is created
counter starts again from 1
```

## How it works

The session middleware is installed with:

```cpp id="xdtwcx"
app.use("/session", middleware::app::adapt_ctx(
  middleware::auth::session(session_options)
));
```

The handler reads the session from request state:

```cpp id="i93hxe"
auto &session =
  req.state<middleware::auth::Session>();
```

The counter is stored as a string:

```cpp id="hudkuo"
session.set("n", std::to_string(n));
```

The value is read with:

```cpp id="i6j8ph"
auto value = session.get("n");
```

The value is removed with:

```cpp id="h5bxox"
session.erase("n");
```

The full session is destroyed with:

```cpp id="kdd82x"
session.destroy();
```

## Session options

The example uses:

```cpp id="a7fla6"
middleware::auth::SessionOptions session_options;

session_options.secret = "dev_session_secret";
session_options.cookie_name = "sid";
session_options.cookie_path = "/";
session_options.secure = false;
session_options.http_only = true;
session_options.same_site = "Lax";
session_options.auto_create = true;
```

Meaning:

```txt id="vaial5"
secret
  secret used by the session middleware

cookie_name
  browser cookie name that stores the session id

cookie_path
  cookie path

secure
  send cookie only over HTTPS when true

http_only
  block JavaScript access to the session cookie

same_site
  browser cross-site cookie policy

auto_create
  create a new session when no valid session exists
```

## Session state fields

The session object contains:

```cpp id="q44osb"
struct Session
{
  std::string id;
  std::unordered_map<std::string, std::string> data;

  bool is_new{false};
  bool dirty{false};
  bool destroyed{false};
};
```

Meaning:

```txt id="qhyrek"
id
  unique session id

data
  key-value session storage

is_new
  true when the session was created for this request

dirty
  true when session data changed

destroyed
  true when the session should be deleted
```

Most handlers only need:

```cpp id="jhz1u7"
session.get(...)
session.set(...)
session.erase(...)
session.destroy()
```

## In-memory store

If no custom store is provided, the dev setup can use an in-memory store.

That is fine for examples.

But in-memory session storage is process-local.

That means:

```txt id="qr2mtn"
sessions disappear when the process restarts
sessions are not shared across multiple server instances
sessions are not suitable for multi-node production by default
```

For production, use a persistent or shared store.

Examples:

```txt id="t0x9ix"
Redis
database-backed store
distributed cache
custom ISessionStore implementation
```

## Custom session store

The session module exposes a store interface:

```cpp id="i62hu8"
class ISessionStore
{
public:
  virtual ~ISessionStore() = default;

  virtual std::optional<Session> load(const std::string &sid) = 0;
  virtual void save(const Session &s, std::chrono::seconds ttl) = 0;
  virtual void destroy(const std::string &sid) = 0;
};
```

You can provide your own store:

```cpp id="bg0chl"
session_options.store = std::make_shared<MySessionStore>();
```

Use this when session data must survive process restarts or be shared across multiple app instances.

## Cookie behavior

The session middleware uses a cookie to identify the session.

In this example:

```txt id="hfwpdf"
sid=...
```

The cookie should usually be:

```txt id="gaz57g"
HttpOnly
Secure in production
SameSite=Lax or Strict
```

The example uses local HTTP development, so:

```cpp id="joukod"
session_options.secure = false;
```

For production HTTPS:

```cpp id="ejkf3j"
session_options.secure = true;
```

## Session lifetime

The options include a TTL:

```cpp id="lcvjsb"
session_options.ttl = std::chrono::hours(24 * 7);
```

This means the session can live for seven days depending on the store behavior.

For sensitive sessions, use a shorter TTL.

For low-risk preferences, longer TTLs can be acceptable.

## Middleware order

The example installs middleware in this order:

```cpp id="icm3i8"
app.use("/session", middleware::app::request_id_dev());
app.use("/session", middleware::app::timing_dev());
app.use("/session", middleware::app::security_headers_dev());

app.use("/session", middleware::app::adapt_ctx(
  middleware::auth::session(session_options)
));
```

The order means:

```txt id="chvql0"
request id
  identifies the request

timing
  measures the request

security headers
  hardens the response

session
  loads or creates session state

handler
  reads and writes session data
```

If the route also receives forms or JSON, install body parsers after broad safety middleware and before the handler.

## Sessions with forms

Sessions are often used with forms.

Example flow:

```txt id="vj7233"
GET /login
  create CSRF token in session

POST /login
  parse form
  validate CSRF token from session
  set user id in session
```

Example session write:

```cpp id="f6f7ez"
session.set("user_id", "123");
```

Example session read:

```cpp id="tgqi2d"
auto user_id = session.get("user_id");
```

If `user_id` exists, the user is logged in.

## Sessions with JSON APIs

Sessions can also be used for browser JSON APIs.

Example:

```txt id="gj4krp"
POST /api/login
  validate credentials
  set session user_id

GET /api/me
  read session user_id
  return current user

POST /api/logout
  destroy session
```

When using sessions from browser JavaScript, cookie and CORS settings matter.

For cross-origin frontend/API setups, you need to configure credentials carefully.

## Security notes

Do not store large data in sessions.

Do not store secrets that do not need to be there.

Good session values:

```txt id="n55nfq"
user_id
cart_id
csrf_token
flash_message
small temporary state
```

Bad session values:

```txt id="bxkyqc"
large JSON payloads
full user profile copies
passwords
private keys
unbounded arrays
large cart contents
```

Store large or durable data in a database.

Keep the session small.

## Production defaults

For production session cookies, prefer:

```txt id="l4abhv"
HttpOnly = true
Secure = true
SameSite = Lax or Strict
short and intentional TTL
persistent shared store
regenerate session id after login
destroy session on logout
```

For local development:

```txt id="hktod3"
Secure = false
```

because the app usually runs on plain HTTP.

For production behind HTTPS:

```txt id="rn7j2i"
Secure = true
```

## Common mistakes

### Using sessions without cookies

A session needs a way to identify the browser.

Usually that means a cookie.

With `curl`, remember:

```txt id="m00y2n"
-c cookies.txt
  save cookies

-b cookies.txt
  send cookies
```

### Forgetting to save the cookie jar

This creates a new session every time:

```bash id="n839hp"
curl -i http://127.0.0.1:8080/session
curl -i http://127.0.0.1:8080/session
```

This keeps the same session:

```bash id="y53m0x"
curl -i -c cookies.txt http://127.0.0.1:8080/session
curl -i -b cookies.txt -c cookies.txt http://127.0.0.1:8080/session
```

### Storing too much data

Sessions should stay small.

Store IDs in the session.

Store large data in the database.

### Using in-memory sessions in production

In-memory sessions disappear when the process restarts.

Use a persistent shared store for real deployments.

### Forgetting Secure in production

For HTTPS production apps:

```cpp id="nkmrzh"
session_options.secure = true;
```

## Complete test flow

Run:

```bash id="vpfg0h"
vix run session_counter.cpp
```

First request:

```bash id="y21quo"
curl -i \
  -c cookies.txt \
  http://127.0.0.1:8080/session
```

Second request:

```bash id="j2q1ay"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/session
```

Read value:

```bash id="uib63n"
curl -i \
  -b cookies.txt \
  http://127.0.0.1:8080/session/value
```

Reset:

```bash id="ez7y7d"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/session/reset
```

Destroy:

```bash id="mdlhme"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/session/destroy
```

Start again:

```bash id="tnvvqk"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/session
```

## Summary

Use sessions when the server needs to remember small state per browser.

Install session middleware:

```cpp id="s9mrqe"
middleware::auth::SessionOptions session_options;

session_options.secret = "dev_session_secret";
session_options.cookie_name = "sid";
session_options.http_only = true;
session_options.same_site = "Lax";

app.use("/session", middleware::app::adapt_ctx(
  middleware::auth::session(session_options)
));
```

Read the session:

```cpp id="o8051b"
auto &session =
  req.state<middleware::auth::Session>();
```

Set a value:

```cpp id="whumwh"
session.set("n", "1");
```

Read a value:

```cpp id="ih46f2"
auto value = session.get("n");
```

Destroy the session:

```cpp id="mvm72q"
session.destroy();
```

The mental model is:

```txt id="u2ofwp"
browser stores sid cookie
server stores session data
request sends sid
middleware loads session
handler reads or writes session
middleware saves session
```
