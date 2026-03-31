# RBAC + Session (Beginner Guide)

This guide shows how to combine **Sessions** (cookie-based login state) with **RBAC** (roles + permissions).

Goal:
- A user "logs in" and we store their identity and authorization in the **session**.
- Protected routes build an `Authz` object from the session.
- Then we enforce:
  - `require_role("admin")`
  - `require_perm("products:write")`

You will get:
- `GET /` (help)
- `POST /login/admin` (creates a session with admin + products:write)
- `POST /login/user` (creates a session with user only)
- `POST /logout` (destroys the session)
- `GET /me` (shows session values)
- `GET /admin` (requires role + perm)

---

## 1) Run

```bash
vix run rbac_session_app.cpp
```

---

## 2) Quick tests (curl)

### 2.1 Public help
```bash
curl -i http://localhost:8080/
```

### 2.2 Login as admin (creates cookie)
Save cookies to a file so next requests reuse the session:

```bash
curl -i -c jar.txt -X POST http://localhost:8080/login/admin
```

### 2.3 Access protected route as admin (OK)
```bash
curl -i -b jar.txt http://localhost:8080/admin
```

### 2.4 Login as normal user (no permission)
```bash
curl -i -c jar.txt -X POST http://localhost:8080/login/user
curl -i -b jar.txt http://localhost:8080/admin
```

Expected: **403** (missing permission).

### 2.5 Inspect current session
```bash
curl -i -b jar.txt http://localhost:8080/me
```

### 2.6 Logout
```bash
curl -i -b jar.txt -X POST http://localhost:8080/logout
curl -i -b jar.txt http://localhost:8080/admin
```

Expected: **401** (not authenticated).

---

## How it works (simple mental model)

- **Session middleware** reads a cookie (example: `sid=...`) and loads a small key/value store.
- We store:
  - `sub` (subject / user id)
  - `roles` (comma-separated, ex: `admin,user`)
  - `perms` (comma-separated, ex: `products:write,orders:read`)
- A small middleware `mw_authz_from_session()` creates an `Authz` object and puts it into request state.
- Then we reuse the existing RBAC middlewares:
  - `require_role("admin")`
  - `require_perm("products:write")`

---

## Full example (copy/paste)

Save as `rbac_session_app.cpp`:

```cpp
/**
 *
 *  @file rbac_session_app.cpp - RBAC + Session combined (Vix.cpp)
 *  @author Gaspard Kirira
 *
 *  Vix.cpp
 *
 *  Run:
 *    vix run rbac_session_app.cpp
 *
 *  Tests:
 *    curl -i http://localhost:8080/
 *    curl -i -c jar.txt -X POST http://localhost:8080/login/admin
 *    curl -i -b jar.txt http://localhost:8080/admin
 *    curl -i -c jar.txt -X POST http://localhost:8080/login/user
 *    curl -i -b jar.txt http://localhost:8080/admin
 *    curl -i -b jar.txt http://localhost:8080/me
 *    curl -i -b jar.txt -X POST http://localhost:8080/logout
 *
 */

#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/presets.hpp>

#include <vix/middleware/auth/session.hpp>
#include <vix/middleware/auth/rbac.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace vix;
namespace J = vix::json;

// ----------------------- tiny helpers (no heavy magic) ------------------------

static std::vector<std::string> split_csv(const std::string &s)
{
  std::vector<std::string> out;
  std::string cur;

  for (char c : s)
  {
    if (c == ',')
    {
      if (!cur.empty())
        out.push_back(cur);
      cur.clear();
      continue;
    }
    // trim simple spaces
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
      cur.push_back(c);
  }

  if (!cur.empty())
    out.push_back(cur);

  // remove empty items
  out.erase(std::remove_if(out.begin(), out.end(),
                           [](const std::string &x)
                           { return x.empty(); }),
            out.end());
  return out;
}

static J::kvs ok(std::string_view msg)
{
  return J::obj({"ok", true, "message", std::string(msg)});
}

static J::kvs err(std::string_view code, int status, std::string_view hint = {})
{
  auto o = J::obj({
      "ok", false,
      "error", std::string(code),
      "status", (long long)status,
  });

  if (!hint.empty())
  {
    // append fields by rebuilding (Simple.hpp style)
    return J::obj({
        "ok", false,
        "error", std::string(code),
        "status", (long long)status,
        "hint", std::string(hint),
    });
  }

  return o;
}

// --------------------- middleware: Authz from Session -------------------------
// Reads session keys and populates vix::middleware::auth::Authz for RBAC checks.

static vix::middleware::MiddlewareFn mw_authz_from_session(bool require_auth = true)
{
  using vix::middleware::auth::Authz;
  using vix::middleware::auth::Session;

  return [require_auth](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    auto *s = ctx.req().try_state<Session>();
    if (!s)
    {
      // Session middleware missing
      ctx.res().status(500).json(err("session_missing", 500,
                                    "Install session middleware before RBAC"));
      return;
    }

    const auto sub = s->get("sub");
    const auto roles = s->get("roles");
    const auto perms = s->get("perms");

    if (require_auth && (!sub || sub->empty()))
    {
      ctx.res().status(401).json(err("unauthorized", 401,
                                    "Login first to create a session"));
      return;
    }

    Authz a;
    a.subject = sub.value_or("");

    if (roles)
      for (auto &r : split_csv(*roles))
        a.roles.insert(std::move(r));

    if (perms)
      for (auto &p : split_csv(*perms))
        a.perms.insert(std::move(p));

    ctx.req().emplace_state<Authz>(std::move(a));
    next();
  };
}

// ------------------------------ routes ----------------------------------------

static void register_routes(App &app)
{
  // Home: explains how to test quickly
  app.get("/", [](Request &, Response &res)
          { res.text(
                "RBAC + Session demo\n\n"
                "POST /login/admin  (sets session: role=admin, perm=products:write)\n"
                "POST /login/user   (sets session: role=user, no products:write)\n"
                "POST /logout       (destroy session)\n"
                "GET  /me           (shows current session)\n"
                "GET  /admin        (requires role=admin + perm=products:write)\n\n"
                "Quick start:\n"
                "  curl -i -c jar.txt -X POST http://localhost:8080/login/admin\n"
                "  curl -i -b jar.txt http://localhost:8080/admin\n"
                "  curl -i -b jar.txt http://localhost:8080/me\n"); });

  // Create an admin session
  app.post("/login/admin", [](Request &req, Response &res)
           {
             auto *s = req.try_state<vix::middleware::auth::Session>();
             if (!s)
             {
               res.status(500).json(err("session_missing", 500));
               return;
             }

             s->set("sub", "user123");
             s->set("roles", "admin");
             s->set("perms", "products:write,orders:read");

             res.json(ok("logged_in_as_admin")); });

  // Create a normal user session (no products:write)
  app.post("/login/user", [](Request &req, Response &res)
           {
             auto *s = req.try_state<vix::middleware::auth::Session>();
             if (!s)
             {
               res.status(500).json(err("session_missing", 500));
               return;
             }

             s->set("sub", "user123");
             s->set("roles", "user");
             s->set("perms", "orders:read");

             res.json(ok("logged_in_as_user")); });

  // Destroy session
  app.post("/logout", [](Request &req, Response &res)
           {
             auto *s = req.try_state<vix::middleware::auth::Session>();
             if (!s)
             {
               res.status(500).json(err("session_missing", 500));
               return;
             }

             s->destroy();
             res.json(ok("logged_out")); });

  // Inspect session values
  app.get("/me", [](Request &req, Response &res)
          {
            auto *s = req.try_state<vix::middleware::auth::Session>();
            if (!s)
            {
              res.status(500).json(err("session_missing", 500));
              return;
            }

            res.json(J::obj({
                "ok", true,
                "sid", s->id,
                "is_new", s->is_new,
                "sub", s->get("sub").value_or(""),
                "roles", s->get("roles").value_or(""),
                "perms", s->get("perms").value_or(""),
            })); });

  // Protected route: requires role + perm
  app.get("/admin", [](Request &req, Response &res)
          {
            auto &a = req.state<vix::middleware::auth::Authz>();

            res.json(J::obj({
                "ok", true,
                "protected", true,
                "sub", a.subject,
                "has_admin", a.has_role("admin"),
                "has_products_write", a.has_perm("products:write"),
            })); });
}

// ------------------------------ install ---------------------------------------

static void install_middlewares(App &app)
{
  using namespace vix::middleware::app;
  using vix::middleware::auth::require_perm;
  using vix::middleware::auth::require_role;

  // 1) Session (global) - cookie-based state
  // Use the preset (dev-friendly). Cookie name defaults to "sid".
  app.use(session_dev("dev_session_secret"));

  // 2) RBAC protection for /admin
  // Build Authz from session, then enforce rules.
  app.use("/admin", chain(
                      adapt_ctx(mw_authz_from_session(true)),
                      adapt_ctx(require_role("admin")),
                      adapt_ctx(require_perm("products:write"))));
}

int main()
{
  App app;

  install_middlewares(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

---

## Notes for real projects

- Do not store sensitive data in session as plain text unless it is protected (signed + secure cookie, HTTPS).
- Behind HTTPS, set `secure=true` and consider `SameSite` based on your frontend setup.
- In production, roles/perms are usually loaded from a database, not hard-coded in `/login/*`.


