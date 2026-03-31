# Advanced Group Patterns

This guide explains advanced architectural patterns using `group()` in Vix.cpp.

These patterns are useful for:

- Large APIs
- Multi-tenant systems
- Versioned APIs
- Modular backend design
- Enterprise-grade structure

---

## 1) Versioned API Pattern (v1 / v2)

Structure:

/api/v1/...
/api/v2/...

Example:

```cpp
App app;

app.group("/api", [&](App::Group& api) {

    api.group("/v1", [&](App::Group& v1) {
        v1.get("/users", [](Request&, Response& res){
            res.json({"version","v1","data","users list"});
        });
    });

    api.group("/v2", [&](App::Group& v2) {
        v2.get("/users", [](Request&, Response& res){
            res.json({"version","v2","data","users list (new schema)"});
        });
    });

});

app.run(8080);
```

Why this is powerful:
- Clean API evolution
- Backward compatibility
- Zero route collision

---

## 2) Multi‑Tenant Group Pattern

Structure:

/tenant/{id}/...

You isolate logic per tenant.

```cpp
app.group("/tenant", [&](App::Group& tenant){

    tenant.get("/{id}/dashboard", [](Request& req, Response& res){
        auto id = req.param("id");
        res.json({"tenant", id});
    });

});
```

Best practice:
- Inject tenant ID into request state via middleware
- Apply tenant‑level RBAC inside the group

---

## 3) Admin Isolation Pattern

Separate public API from admin API.

```cpp
app.group("/admin", [&](App::Group& admin){

    admin.use(middleware::app::jwt_auth("secret"));
    admin.use(middleware::app::rbac_admin());

    admin.get("/stats", [](Request&, Response& res){
        res.json({"admin", true});
    });

});
```

Benefits:
- All admin logic isolated
- Security applied once
- No repetition

---

## 4) Feature Module Mounting

Simulate plugin-style mounting.

```cpp
void mountUsers(App::Group& api)
{
    api.group("/users", [&](App::Group& users){
        users.get("/", [](Request&, Response& res){
            res.send("Users list");
        });
    });
}

int main()
{
    App app;
    auto api = app.group("/api");

    mountUsers(api);

    app.run(8080);
}
```

This allows:
- Clean separation by domain
- Team-based development
- Independent modules

---

## 5) Security Layer Stacking

Order matters:

Headers → CORS → Auth → RBAC → Rate limit

```cpp
api.use(middleware::app::security_headers_dev());
api.use(middleware::app::cors_dev());
api.use(middleware::app::jwt_auth("secret"));
api.use(middleware::app::rbac_admin());
api.use(middleware::app::rate_limit_dev(60, std::chrono::minutes(1)));
```

This ensures:
- Preflight handled correctly
- Auth before permission checks
- Protection against abuse

---

## 6) Enterprise Folder Architecture

Recommended structure:

```
/src
  /api
    v1_users.cpp
    v1_orders.cpp
    v2_users.cpp
  /admin
    dashboard.cpp
  /modules
    billing.cpp
    analytics.cpp
```

Each file exports a mount function:

```cpp
void mount(App::Group& group);
```

Main app only wires modules together.

---

## Key Takeaway

`group()` is not just route prefixing.

It is an architectural boundary:

- Security boundary
- Version boundary
- Tenant boundary
- Domain boundary

Mastering groups = building scalable backend systems.

---

Vix.cpp Advanced Routing Patterns.

