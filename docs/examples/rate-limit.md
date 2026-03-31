# Rate Limiting (Beginner Guide)

Welcome 👋\
This page teaches you **rate limiting** in Vix.cpp with **very small
examples**.

Rate limiting helps you protect your API against:

-   spam and brute-force attacks
-   abusive clients
-   accidental traffic bursts
-   expensive endpoints being called too often

## What is rate limiting?

Think of a bucket of tokens:

-   The bucket has a maximum size = **capacity** (burst)
-   Every request consumes **1 token**
-   Tokens can refill over time = **refill_per_sec**
-   If the bucket is empty, the server returns **429 Too Many Requests**

Vix middleware implements this idea for you.

# 1) Minimal rate limit on `/api`

This is the smallest server: only `/api/*` is rate limited.

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // capacity=5, refill=0 => the 6th request gets 429 (easy demo)
  app.use("/api", middleware::app::rate_limit_custom_dev(5.0, 0.0));

  app.get("/", [](Request &, Response &res) {
    res.send("public route");
  });

  app.get("/api/ping", [](Request &req, Response &res) {
    res.json({
      "ok", true,
      "msg", "pong",
      "xff", req.header("x-forwarded-for")
    });
  });

  app.run(8080);
}
```

### Test with curl

Run the server:

``` bash
vix run rate_limit_server.cpp
```

Call it 6 times:

``` bash
for i in $(seq 1 6); do
  echo "---- $i"
  curl -i http://localhost:8080/api/ping
done
```

Expected:

-   first 5 requests: **200**
-   request 6: **429 Too Many Requests**

# 2) Production-like preset: `rate_limit_dev(capacity, window)`

If you want a simpler mental model:

-   **capacity** = how many requests you allow
-   **window** = time window

Example: 60 requests per minute.

``` cpp
#include <chrono>
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // 60 requests per minute per client key (default key_header: x-forwarded-for)
  app.use("/api", middleware::app::rate_limit_dev(
      60,
      std::chrono::minutes(1)
  ));

  app.get("/api/ping", [](Request &, Response &res) {
    res.json({"ok", true, "msg", "pong"});
  });

  app.run(8080);
}
```

### Important

By default the limiter uses **x-forwarded-for** as client key.

In dev you can simulate client IPs by sending that header:

``` bash
curl -i http://localhost:8080/api/ping -H "X-Forwarded-For: 9.9.9.9"
```

# 3) Custom key: limit by your own header

Sometimes you want to limit by:

-   user id
-   API key
-   session id
-   tenant id

You can pass a header name as key:

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // limit per "x-user-id"
  app.use("/api", middleware::app::rate_limit_custom_dev(
      10.0,   // capacity
      1.0,    // refill per second
      "x-user-id"
  ));

  app.get("/api/ping", [](Request &req, Response &res) {
    res.json({
      "ok", true,
      "user", req.header("x-user-id")
    });
  });

  app.run(8080);
}
```

Test:

``` bash
curl -i http://localhost:8080/api/ping -H "x-user-id: alice"
curl -i http://localhost:8080/api/ping -H "x-user-id: bob"
```

Each user has their own bucket.

# 4) Real production pattern: CORS + IP filter + rate limit (order matters)

This is your combined example (short explanation):

-   CORS runs first (blocks bad origins)
-   IP filter runs second (blocks denied IPs)
-   Rate limit runs third (protects the allowed traffic)

``` cpp
app.use("/api", middleware::app::cors_ip_demo());
app.use("/api", middleware::app::ip_filter_dev("x-vix-ip", {"1.2.3.4"}));
app.use("/api", middleware::app::rate_limit_custom_dev(
  5.0, 0.0, "x-vix-ip"
));
```

### Why explicit OPTIONS routes?

Browsers send a **preflight** request (OPTIONS).\
If OPTIONS is auto-handled before middleware runs, you may miss CORS
headers.

So you add:

``` cpp
app.options("/api/ping", [](Request&, Response& res){
  res.status(204).send();
});
```

# Common beginner mistakes

1)  Forgetting that rate limiting needs a stable key\
    If your key header changes on every request, the limiter becomes
    useless.

2)  Using `X-Forwarded-For` without a reverse proxy\
    In real deployments, your reverse proxy (nginx, cloud LB) sets this
    header. In local tests, you can set it manually with curl.

3)  Putting rate limit before IP filter (or auth)\
    Usually you want to reject denied requests early, then rate limit
    what is allowed.

# Summary

-   Use `rate_limit_custom_dev(capacity, refill_per_sec, key_header)`
    when you want full control.
-   Use `rate_limit_dev(capacity, window)` for the simple "N requests
    per time window" model.
-   Always think about the **key** (IP, user id, api key, etc.).


