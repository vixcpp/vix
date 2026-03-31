# API Key Middleware Guide (Vix.cpp) — Beginner Friendly

## What is an API Key?

An API key is a simple secret string sent by the client to prove it is allowed to access a protected endpoint.

It is NOT as powerful as JWT, but it is:

- Simple
- Fast
- Perfect for internal APIs or microservices

---

# How API Key Works in Vix.cpp

Client sends:

Header:
```bash
    x-api-key: secret
```
OR

Query param:
```bash
    ?api_key=secret
```
Middleware checks:
- Is key present?
- Is key valid?
- If yes → continue
- If no → 401 or 403

---

# Minimal Example

File: `api_key_app_simple.cpp`

```cpp
App app;

// Protect only /secure
app.use("/secure", middleware::app::api_key_dev("secret"));

app.get("/secure", [](Request& req, Response& res)
{
    auto& key = req.state<vix::middleware::auth::ApiKey>();

    res.json({
        "ok", true,
        "api_key", key.value
    });
});
```

# Run

```bash
vix run api_key_app_simple.cpp
```

Server runs on:
```bash
http://localhost:8080
```

# Test With curl

## 1 Missing key

```bash
curl -i http://localhost:8080/secure
```

Result:
401 Unauthorized

---

## 2) Invalid key

```bash
curl -i -H "x-api-key: wrong" http://localhost:8080/secure
```

Result:
403 Forbidden

---

## 3) Valid key (Header)

```bash
curl -i -H "x-api-key: secret" http://localhost:8080/secure
```

Result:
200 OK

---

## 4) Valid key (Query param)

```bash
curl -i "http://localhost:8080/secure?api_key=secret"
```

Result:
200 OK

---

# What Happens Internally?

If key is valid:
```cpp
req.state<ApiKey>().value
```
contains:

"secret"

You can use it inside your route.

---

# When Should You Use API Key?

Good for:

- Internal service-to-service auth
- Dev environments
- Small private APIs

Not ideal for:

- Complex user systems
- Role-based permissions
- Public user authentication

---

# Full Working Example

```cpp
#include <iostream>
#include <string>

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
    App app;

    // Protect /secure
    app.use("/secure", middleware::app::api_key_dev("secret"));

    // Public route
    app.get("/", [](Request&, Response& res)
    {
        res.send("API Key example: /secure requires x-api-key: secret");
    });

    // Protected route
    app.get("/secure", [](Request& req, Response& res)
    {
        auto& key = req.state<vix::middleware::auth::ApiKey>();

        res.json({
            "ok", true,
            "api_key", key.value
        });
    });

    app.run(8080);
    return 0;
}
```

---

# Summary

API Key = Simple shared secret

Very fast
Very lightweight
Very easy to understand

Vix.cpp keeps it clean and minimal.


