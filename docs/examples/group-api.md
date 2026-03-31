# Route Groups Guide

This guide explains how to use **route groups** in Vix.cpp.

Groups let you:

- Share a common URL prefix
- Apply middleware to multiple routes at once
- Organize APIs cleanly
- Nest sub-groups (ex: /api/admin)

---

# 1) What is a Route Group?

Instead of writing:
```bash
    /api/users
    /api/products
    /api/admin/dashboard
```
You define:
```cpp
    app.group("/api", ...)
```
And all routes inside automatically start with `/api`.

---

# 2) Basic Group Example

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.group("/api", [&](App::Group &api)
  {
      api.get("/ping", [](Request&, Response& res){
          res.json({"ok", true});
      });
  });

  app.run(8080);
}
```

Now:
```bash
    GET /api/ping
```
---

# 3) Protecting a Route Inside a Group

You can protect only one route:

```cpp
api.protect("/secure", middleware::app::api_key_dev("secret"));

api.get("/secure", [](Request &req, Response &res)
{
    auto &k = req.state<vix::middleware::auth::ApiKey>();
    res.json({
        "ok", true,
        "api_key", k.value
    });
});
```

Test:
```bash
    curl -i http://localhost:8080/api/secure
    curl -i -H "x-api-key: secret" http://localhost:8080/api/secure
```
---

# 4) Nested Groups (Admin Example)

Groups can be nested:

```cpp
api.group("/admin", [&](App::Group &admin)
{
    admin.use(middleware::app::jwt_auth("dev_secret"));
    admin.use(middleware::app::rbac_admin());

    admin.get("/dashboard", [](Request &req, Response &res)
    {
        auto &authz = req.state<vix::middleware::auth::Authz>();
        res.json({
            "ok", true,
            "sub", authz.subject,
            "role", "admin"
        });
    });
});
```

This creates:
```bash
    /api/admin/dashboard
```
All admin routes share the same middleware.

---

# 5) Builder Style Group

You can also create a group first:

```cpp
auto api = app.group("/api");

api.get("/public", [](Request &, Response &res)
{
    res.send("Public endpoint");
});

api.use(middleware::app::api_key_dev("secret"));

api.get("/secure", [](Request &req, Response &res)
{
    auto &k = req.state<vix::middleware::auth::ApiKey>();
    res.json({"ok", true});
});
```

Everything after `api.use(...)` is protected.

---

# 6) When to Use Groups

Use groups when:

- You build REST APIs
- You want prefix-based protection
- You need nested areas (admin, internal, public)
- You want clean structure

---

# 7) Recommended Production Pattern

Typical structure:
```bash
    /api
      /public
      /auth
      /admin
      /internal
```
Example:
```cpp
    app.group("/api", ...)
        api.group("/admin", ...)
        api.group("/auth", ...)
```
Keep middleware close to the group it protects.

---

# Summary

Groups in Vix.cpp give you:

- Clean URL organization
- Middleware inheritance
- Nested API design
- Scalable backend structure

They are essential for real-world applications.

