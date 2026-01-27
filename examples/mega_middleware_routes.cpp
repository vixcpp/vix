/**
 *
 *  @file examples/http_middleware/mega_middleware_routes.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  ----------------------------------------------------------------------------
 *  GOAL
 *  ----
 *  A single, big example file you can drop into:
 *    vix/examples/http_middleware/mega_middleware_routes.cpp
 *
 *  It demonstrates:
 *    - App routes (GET/POST/etc.)
 *    - App-level middleware (App::use / prefix install)
 *    - Context-based middleware via adapt_ctx()
 *    - Legacy HttpMiddleware via adapt()
 *    - Middleware chaining + conditional gating (when/protect_prefix)
 *    - Security: CORS, Rate limit, CSRF, Security headers, IP filter
 *    - Parsers: JSON, Form, Multipart, Multipart Save
 *    - Auth: API key, JWT, RBAC (admin)
 *    - HTTP cache middleware (GET cache)
 *    - RequestState usage (store data across middlewares+handler)
 *    - Error patterns: early return, status+payload returnable, res.send/json
 *
 *  NOTES
 *  -----
 *  This file intentionally repeats patterns and includes many routes so you can
 *  screenshot it and say: "this is how you write simple routes + middlewares".
 *
 *  You can start it with:
 *    vix run examples/http_middleware/mega_middleware_routes.cpp
 *
 *  Then test quickly:
 *    curl -i http://127.0.0.1:8080/
 *    curl -i http://127.0.0.1:8080/api/ping
 *    curl -i -H "x-api-key: dev_key_123" http://127.0.0.1:8080/api/secure/whoami
 *    curl -i http://127.0.0.1:8080/dev/trace
 *    curl -i -H "x-vix-cache: bypass" http://127.0.0.1:8080/api/cache/demo
 *
 */

// ----------------------------- includes --------------------------------------
#include <vix.hpp>

// If your tree exposes these headers exactly as in your snippets:
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/presets.hpp>
#include <vix/middleware/app/http_cache.hpp>

// Some projects place these in different paths; keep includes minimal.
// ----------------------------------------------------------------------------

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace vix;
namespace J = vix::json;

// ----------------------------- tiny helpers ----------------------------------

static long long now_ms_wall()
{
  using namespace std::chrono;
  const auto t = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
  return static_cast<long long>(t);
}

static J::kvs ok_msg(std::string_view msg)
{
  return J::obj({
      "ok",
      true,
      "message",
      std::string(msg),
  });
}

static J::kvs err_msg(std::string_view msg, int status = 400)
{
  return J::obj({
      "ok",
      false,
      "status",
      (long long)status,
      "error",
      std::string(msg),
  });
}

// ------------------------- RequestState demo types ---------------------------
// RequestState is type-based storage (std::any). We'll store:
// - RequestId
// - AuthInfo
// - ParsedBody indicator
// - Debug timings

struct RequestId
{
  std::string value;
};

struct AuthInfo
{
  bool authed{false};
  std::string subject;
  std::string role;
};

struct ParseInfo
{
  bool parsed_json{false};
  bool parsed_form{false};
  bool parsed_multipart{false};
};

struct TimingInfo
{
  long long t0_ms{0};
  long long t1_after_global_mw{0};
  long long t2_after_api_mw{0};
};

// ----------------------------- demo middleware --------------------------------
// 1) A very small "request id" middleware (Context-based style).
//    Stores a RequestId in RequestState and sets a header.
//    This is meant to be adapted with adapt_ctx().

static vix::middleware::MiddlewareFn mw_request_id()
{
  return [](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    // Generate a simple id: <ms>-<ptr>
    RequestId rid;
    rid.value = std::to_string(now_ms_wall()) + "-" + std::to_string((std::uintptr_t)(&ctx));

    ctx.req().emplace_state<RequestId>(std::move(rid));
    ctx.res().header("x-request-id", ctx.req().state<RequestId>().value);

    next(); // continue
  };
}

// 2) Global timer middleware (Context-based). Stores t0.
static vix::middleware::MiddlewareFn mw_timing_start()
{
  return [](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    TimingInfo t;
    t.t0_ms = now_ms_wall();
    ctx.req().emplace_state<TimingInfo>(t);
    next();
  };
}

// 3) After-middleware "mark" (Context-based). Updates timing.
static vix::middleware::MiddlewareFn mw_timing_mark_global()
{
  return [](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    next();

    if (ctx.req().has_state_type<TimingInfo>())
    {
      auto &t = ctx.req().state<TimingInfo>();
      t.t1_after_global_mw = now_ms_wall();
    }
  };
}

// 4) A strict "require header" middleware (legacy HttpMiddleware style).
//    - If header missing: 401 and DO NOT call next()
//    - Otherwise: next()
static vix::middleware::HttpMiddleware mw_require_header(std::string header, std::string expected)
{
  return [header = std::move(header), expected = std::move(expected)](
             vix::Request &req, vix::Response &res, vix::middleware::Next next) mutable
  {
    const std::string got = req.header(header);
    if (got.empty() || got != expected)
    {
      res.status(401).json(J::obj({
          "ok",
          false,
          "error",
          "unauthorized",
          "hint",
          "Missing or invalid header",
          "required_header",
          header,
      }));
      return;
    }
    next();
  };
}

// 5) A tiny "fake auth" middleware (Context-based).
//    If "x-user" is present, treat as authenticated.
//    If "x-role" is present, use it.
//    Stores AuthInfo in RequestState.
//    (This is purely for example. Real auth should use jwt/api_key middlewares.)
static vix::middleware::MiddlewareFn mw_fake_auth()
{
  return [](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    AuthInfo info;
    const std::string user = ctx.req().header("x-user");
    const std::string role = ctx.req().header("x-role");

    if (!user.empty())
    {
      info.authed = true;
      info.subject = user;
      info.role = role.empty() ? "user" : role;
    }

    ctx.req().emplace_state<AuthInfo>(std::move(info));
    next();
  };
}

// 6) Require "admin" role from stored AuthInfo
static vix::middleware::MiddlewareFn mw_require_admin()
{
  return [](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    if (!ctx.req().has_state_type<AuthInfo>())
    {
      ctx.res().status(401).json(J::obj({"ok", false, "error", "unauthorized"}));
      return;
    }

    const auto &a = ctx.req().state<AuthInfo>();
    if (!a.authed)
    {
      ctx.res().status(401).json(J::obj({"ok", false, "error", "unauthorized"}));
      return;
    }

    if (a.role != "admin")
    {
      ctx.res().status(403).json(J::obj({
          "ok",
          false,
          "error",
          "forbidden",
          "hint",
          "admin role required",
      }));
      return;
    }

    next();
  };
}

// 7) ParseInfo marker middleware (Context-based) for JSON/Form/Multipart.
//    This is for showing how state can be set by parser middlewares.
static vix::middleware::MiddlewareFn mw_mark_parsed_json()
{
  return [](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    if (!ctx.req().has_state_type<ParseInfo>())
      ctx.req().emplace_state<ParseInfo>(ParseInfo{});

    auto &p = ctx.req().state<ParseInfo>();
    p.parsed_json = true;
    next();
  };
}

static vix::middleware::MiddlewareFn mw_mark_parsed_form()
{
  return [](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    if (!ctx.req().has_state_type<ParseInfo>())
      ctx.req().emplace_state<ParseInfo>(ParseInfo{});

    auto &p = ctx.req().state<ParseInfo>();
    p.parsed_form = true;
    next();
  };
}

static vix::middleware::MiddlewareFn mw_mark_parsed_multipart()
{
  return [](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    if (!ctx.req().has_state_type<ParseInfo>())
      ctx.req().emplace_state<ParseInfo>(ParseInfo{});

    auto &p = ctx.req().state<ParseInfo>();
    p.parsed_multipart = true;
    next();
  };
}

// ------------------------------ routes ----------------------------------------
// We keep routes in functions so main() stays clean (your preference).

static void register_public_routes(vix::App &app)
{
  // GET /
  app.get("/", [](Request &, Response &res)
          { res.json(J::obj({
                "message",
                "Vix.cpp middleware mega example 👋",
                "hint",
                "Try /api/ping, /api/cache/demo, /api/secure/whoami, /dev/trace",
            })); });

  // GET /hello
  app.get("/hello", [](Request &, Response &res)
          { res.json(ok_msg("Hello, Vix!")); });

  // GET /txt
  app.get("/txt", [](Request &, Response &)
          { return "Hello world"; });

  // GET /mix (shows "res.send()" takes precedence and return payload is ignored)
  app.get("/mix", [](Request &, Response &res)
          {
            res.status(201).send("Created");
            return J::obj({"ignored", true}); });

  // GET /users/{id} -> 404
  app.get("/users/{id}", [](Request &, Response &res)
          { res.status(404).json(err_msg("User not found", 404)); });
}

static void register_api_routes(vix::App &app)
{
  // GET /api/ping
  app.get("/api/ping", [](Request &, Response &res)
          {
            const bool hasRid = res.has_header(boost::beast::http::field::unknown)
                                ? false
                                : true;
            (void)hasRid;
            res.json(J::obj({
                "ok", true,
                "pong", true,
                "ts_ms", (long long)now_ms_wall(),
            })); });

  // GET /api/who
  app.get("/api/who", [](Request &req, Response &res)
          {
            // RequestState demo
            std::string rid = req.has_state_type<RequestId>() ? req.state<RequestId>().value : "";

            bool authed = false;
            std::string subject = "anonymous";
            std::string role = "guest";

            if (req.has_state_type<AuthInfo>())
            {
              const auto &a = req.state<AuthInfo>();
              authed = a.authed;
              if (a.authed)
              {
                subject = a.subject;
                role = a.role;
              }
            }

            res.json(J::obj({
                "ok", true,
                "request_id", rid,
                "authed", authed,
                "subject", subject,
                "role", role,
            })); });

  // GET /api/cache/demo
  // Hits/miss should appear via x-vix-cache-status
  app.get("/api/cache/demo", [](Request &, Response &res)
          { res.json(J::obj({
                "ok",
                true,
                "cache_demo",
                true,
                "value",
                "same response can be cached",
                "ts_ms",
                (long long)now_ms_wall(),
            })); });

  // GET /api/cache/heavy
  app.get("/api/cache/heavy", [](Request &, Response &)
          {
            // Fake heavy payload
            std::vector<J::token> items;
            items.reserve(64);
            for (int i = 0; i < 64; ++i)
            {
              items.emplace_back(J::obj({
                  "id", (long long)i,
                  "name", std::string("item-") + std::to_string(i),
                  "ts_ms", (long long)now_ms_wall(),
              }));
            }

            return J::obj({
                "ok", true,
                "items", J::array(std::move(items)),
            }); });

  // POST /api/echo/json (expects JSON parser middleware)
  app.post("/api/echo/json", [](Request &req, Response &res)
           {
             // If your JSON parser stores parsed json in state (store_in_state=true),
             // you'd read it from RequestState. Since type name depends on parser impl,
             // we just demonstrate raw body + safe fallback.
             const std::string body = req.body();
             res.json(J::obj({
                 "ok", true,
                 "kind", "json",
                 "raw_body", body,
             })); });

  // POST /api/echo/form (expects form parser)
  app.post("/api/echo/form", [](Request &req, Response &res)
           { res.json(J::obj({
                 "ok",
                 true,
                 "kind",
                 "form",
                 "raw_body",
                 req.body(),
             })); });

  // POST /api/echo/multipart (expects multipart_save)
  app.post("/api/echo/multipart", [](Request &, Response &res)
           { res.json(J::obj({
                 "ok",
                 true,
                 "kind",
                 "multipart",
                 "hint",
                 "multipart_save middleware should store files info in state",
             })); });

  // GET /api/secure/whoami (protected by API key + fake auth for demo)
  app.get("/api/secure/whoami", [](Request &req, Response &res)
          {
            std::string rid = req.has_state_type<RequestId>() ? req.state<RequestId>().value : "";
            res.json(J::obj({
                "ok", true,
                "secure", true,
                "request_id", rid,
                "message", "You passed API key middleware",
            })); });

  // GET /api/admin/stats (protected by fake admin role middleware)
  app.get("/api/admin/stats", [](Request &req, Response &res)
          {
            long long t0 = 0;
            long long t1 = 0;
            long long t2 = 0;

            if (req.has_state_type<TimingInfo>())
            {
              const auto &t = req.state<TimingInfo>();
              t0 = t.t0_ms;
              t1 = t.t1_after_global_mw;
              t2 = t.t2_after_api_mw;
            }

            res.json(J::obj({
                "ok", true,
                "admin", true,
                "timing", J::obj({
                    "t0_ms", t0,
                    "t1_after_global_mw_ms", t1,
                    "t2_after_api_mw_ms", t2,
                }),
            })); });
}

static void register_dev_routes(vix::App &app)
{
  // GET /dev/trace
  app.get("/dev/trace", [](Request &req, Response &res)
          {
            std::string rid = req.has_state_type<RequestId>() ? req.state<RequestId>().value : "";

            bool hasParse = req.has_state_type<ParseInfo>();
            ParseInfo p{};
            if (hasParse)
              p = req.state<ParseInfo>();

            res.json(J::obj({
                "ok", true,
                "dev", true,
                "request_id", rid,
                "parse_info", J::obj({
                    "parsed_json", p.parsed_json,
                    "parsed_form", p.parsed_form,
                    "parsed_multipart", p.parsed_multipart,
                }),
                "hint", "Use /api/echo/json or /api/echo/form to see parse markers",
            })); });

  // GET /dev/boom (to see exception handling in dev html maybe)
  app.get("/dev/boom", [](Request &, Response &)
          {
            throw std::runtime_error("boom: dev crash example");
            return "unreachable"; });
}

// ---------------------------- middleware install ------------------------------
// We demonstrate:
// - Global middlewares
// - Prefix middlewares
// - Conditional middlewares
// - Chains

static void install_global_middlewares(vix::App &app)
{
  using namespace vix::middleware::app;

  // (A) request id + timing start, as ctx-based middleware
  app.use(adapt_ctx(mw_request_id()));
  app.use(adapt_ctx(mw_timing_start()));

  // (B) security headers (ctx-based preset)
  app.use(security_headers_dev(false));

  // (C) permissive CORS for dev
  app.use(cors_dev());

  // (D) fake auth (for demo)
  app.use(adapt_ctx(mw_fake_auth()));

  // (E) mark timing after global middlewares
  app.use(adapt_ctx(mw_timing_mark_global()));
}

static void install_api_middlewares(vix::App &app)
{
  using namespace vix::middleware::app;

  // Everything under /api/ gets:
  // - rate limit
  // - http cache (GET) for /api/cache/*
  // - parsers (json/form/multipart) only where needed
  // - API key for /api/secure/*
  // - admin guard for /api/admin/*
  //
  // We'll do it with prefix install + protect_prefix.

  // 1) Rate limit for /api/
  install(app, "/api/", rate_limit_dev(120, std::chrono::minutes(1)));

  // 2) HTTP cache on /api/cache/
  //    cfg.prefix is used by install_http_cache, but you also have install().
  //    We'll use install_http_cache for the prefix.
  {
    HttpCacheAppConfig cfg;
    cfg.prefix = "/api/cache/";
    cfg.only_get = true;
    cfg.ttl_ms = 25'000;
    cfg.allow_bypass = true;
    cfg.bypass_header = "x-vix-cache";
    cfg.bypass_value = "bypass";
    cfg.add_debug_header = true;
    cfg.debug_header = "x-vix-cache-status";
    cfg.vary_headers = {"accept-encoding", "accept"};

    install_http_cache(app, std::move(cfg));
  }

  // 3) Mark "api middleware reached" timing (ctx-based)
  install(app, "/api/", adapt_ctx([](vix::middleware::Context &ctx, vix::middleware::Next next)
                                  {
                                    next();
                                    if (ctx.req().has_state_type<TimingInfo>())
                                    {
                                      ctx.req().state<TimingInfo>().t2_after_api_mw = now_ms_wall();
                                    } }));

  // 4) Parsers:
  //    For demo, we install JSON parser only on /api/echo/json
  //    and Form parser only on /api/echo/form, Multipart save on /api/echo/multipart
  //
  // Your presets json_dev/form_dev/multipart_save_dev create adapt_ctx wrappers already.
  install_exact(app, "/api/echo/json", chain(json_dev(1024, true, true), adapt_ctx(mw_mark_parsed_json())));
  install_exact(app, "/api/echo/form", chain(form_dev(1024, true), adapt_ctx(mw_mark_parsed_form())));
  install_exact(app, "/api/echo/multipart", chain(multipart_save_dev("uploads", 10 * 1024 * 1024), adapt_ctx(mw_mark_parsed_multipart())));

  // 5) Protect /api/secure/ with API key (preset)
  install(app, "/api/secure/", api_key_dev("dev_key_123"));

  // 6) Protect /api/admin/ with "fake admin" chain
  //    Require x-user and x-role: admin
  install(app, "/api/admin/", chain(adapt_ctx(mw_fake_auth()), adapt_ctx(mw_require_admin())));

  // 7) Example: legacy HttpMiddleware adaptation (header gate) on exact path
  //    Protect /api/ping with x-demo: 1
  install_exact(app, "/api/ping", adapt(mw_require_header("x-demo", "1")));
}

static void install_dev_middlewares(vix::App &app)
{
  using namespace vix::middleware::app;

  // Example IP filter under /dev/
  install(app, "/dev/", ip_filter_dev("x-vix-ip", {"1.2.3.4"}, true));

  // Example CSRF (dev) for /dev/ (usually for browser forms)
  // install(app, "/dev/", csrf_dev());
  // (left commented to avoid blocking GETs if your csrf is strict)
}

// --------------------------------- main ---------------------------------------

int main()
{
  vix::App app;

  // ----------------------------
  // middlewares (global + prefix)
  // ----------------------------
  install_global_middlewares(app);
  install_api_middlewares(app);
  install_dev_middlewares(app);

  // ----------------------------
  // routes
  // ----------------------------
  register_public_routes(app);
  register_api_routes(app);
  register_dev_routes(app);

  // ----------------------------
  // extra "documentation route"
  // ----------------------------
  app.get("/_routes", [](Request &, Response &res)
          {
            std::vector<J::token> routes;

            auto push = [&](std::string_view method, std::string_view path, std::string_view note)
            {
              routes.emplace_back(J::obj({
                  "method", std::string(method),
                  "path", std::string(path),
                  "note", std::string(note),
              }));
            };

            push("GET", "/", "public hello");
            push("GET", "/hello", "simple route");
            push("GET", "/txt", "returns const char* (auto-send)");
            push("GET", "/mix", "res.send() wins over returned payload");

            push("GET", "/api/ping", "needs header x-demo: 1 (legacy middleware adapted)");
            push("GET", "/api/who", "shows RequestState (AuthInfo/RequestId)");
            push("GET", "/api/cache/demo", "GET cache (x-vix-cache-status: hit/miss/bypass)");
            push("GET", "/api/cache/heavy", "bigger payload cached");
            push("POST", "/api/echo/json", "json parser + parse marker");
            push("POST", "/api/echo/form", "form parser + parse marker");
            push("POST", "/api/echo/multipart", "multipart save + parse marker");
            push("GET", "/api/secure/whoami", "requires x-api-key: dev_key_123");
            push("GET", "/api/admin/stats", "requires x-user + x-role: admin");

            push("GET", "/dev/trace", "debug route; may be IP-filtered");
            push("GET", "/dev/boom", "throws to test dev error handling");

            res.json(J::obj({
                "ok", true,
                "count", (long long)routes.size(),
                "routes", J::array(std::move(routes)),
                "tips", J::array({
                    "Use -i with curl to see x-request-id and x-vix-cache-status headers",
                    "For /api/ping add: -H 'x-demo: 1'",
                    "For /api/secure/whoami add: -H 'x-api-key: dev_key_123'",
                    "For /api/admin/stats add: -H 'x-user: gaspard' -H 'x-role: admin'",
                    "To bypass cache: -H 'x-vix-cache: bypass'",
                }),
            })); });

  // ----------------------------
  // run
  // ----------------------------
  app.run(8080);
  return 0;
}
