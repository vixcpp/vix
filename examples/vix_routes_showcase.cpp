/**
 *
 *  @file vix_routes_showcase.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp — Routes Showcase (examples/)
 *
 *  Goal:
 *    A single, large, self-contained file that showcases how routes look in Vix.cpp:
 *      - simple routes
 *      - JSON responses (flat + nested)
 *      - status codes
 *      - path params (/users/{id})
 *      - query params (?page=1&limit=20)
 *      - headers
 *      - request body + JSON parsing
 *      - mixed behaviors (send + return)
 *      - “patterns” you can copy/paste for real projects
 *
 *  Notes:
 *    - This file intentionally focuses on the public routing API style (how you write routes),
 *      not the internal implementation.
 *    - You can keep adding more routes here as your “living playground”.
 *
 *  Tested style:
 *    - main() contains no business logic: it only calls functions.
 *
 *  Vix.cpp
 *
 */
// ============================================================================
// QUICK MAP (a lot of GET routes)
// ----------------------------------------------------------------------------
// GET  /                        -> {"message":"Hello, Vix!"}
// GET  /health                  -> {"ok":true,"service":"vix","ts":"..."}
// GET  /txt                     -> plain text
// GET  /status/{code}           -> returns given status with JSON payload
// GET  /users/{id}              -> shows path params usage
// GET  /search?q=...            -> shows query usage
// GET  /headers                 -> echoes some request headers
// GET  /echo/body               -> dumps raw request body (useful for debug)
// GET  /echo/json               -> parses JSON body and returns it
// GET  /demo/...                -> lots of small patterns
// ============================================================================

#include <vix.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <utility>

using namespace vix;
namespace J = vix::json;

// ---------------------------------------------------------------------------
// Small helpers (keep the route bodies clean)
// ---------------------------------------------------------------------------

static inline int to_int_or(std::string_view s, int fallback)
{
  if (s.empty())
    return fallback;
  int sign = 1;
  std::size_t i = 0;

  if (s[0] == '-')
  {
    sign = -1;
    i = 1;
  }
  if (i >= s.size())
    return fallback;

  long long v = 0;
  for (; i < s.size(); ++i)
  {
    const char c = s[i];
    if (c < '0' || c > '9')
      return fallback;
    v = (v * 10) + (c - '0');
    if (v > 2000000000LL)
      break;
  }
  v *= sign;
  if (v < -2147483648LL || v > 2147483647LL)
    return fallback;
  return static_cast<int>(v);
}

static inline bool to_bool(std::string_view s)
{
  if (s == "1" || s == "true" || s == "yes" || s == "on")
    return true;
  return false;
}

static inline std::string take_or(std::string_view v, std::string fallback)
{
  return v.empty() ? fallback : std::string(v);
}

// ---------------------------------------------------------------------------
// Routes registration (grouped by themes)
// ---------------------------------------------------------------------------

static void register_basic_routes(App &app)
{
  // Home
  app.get("/", [](Request &, Response &res)
          { res.json({"message", "Hello, Vix!"}); });

  // Simple hello
  app.get("/hello", [](const Request &, Response &)
          {
    // Returning a payload => auto-send
    return vix::json::o("message", "Hello", "id", 20); });

  // Plain text
  app.get("/txt", [](const Request &, Response &)
          { return "Hello world"; });

  // Health (timestamp string can be added later; here we keep it minimal)
  app.get("/health", [](Request &, Response &res)
          { res.json({"ok", true, "service", "vix"}); });

  // Simple nested JSON
  app.get("/user", [](Request &, Response &res)
          { res.json({"name", "Ada",
                      "tags", J::array({"c++", "net", "http"}),
                      "profile", J::obj({"id", 42, "vip", true})}); });
}

static void register_status_routes(App &app)
{
  // Dynamic status code from path
  app.get("/status/{code}", [](Request &req, Response &res)
          {
    const std::string codeStr = req.param("code", "200");
    const int code = to_int_or(codeStr, 200);

    res.status(code).json({
      "status", code,
      "ok", (code >= 200 && code < 300),
      "hint", "Try /status/404 or /status/201"
    }); });

  // Common status samples
  app.get("/status/ok", [](Request &, Response &res)
          { res.status(200).json({"status", 200, "message", "OK"}); });

  app.get("/status/created", [](Request &, Response &res)
          { res.status(201).json({"status", 201, "message", "Created"}); });

  app.get("/status/not-found", [](Request &, Response &res)
          { res.status(404).json({"status", 404, "error", "Not Found"}); });

  app.get("/status/error", [](Request &, Response &res)
          { res.status(500).json({"status", 500, "error", "Internal Server Error"}); });
}

static void register_path_param_routes(App &app)
{
  // Basic path param
  app.get("/users/{id}", [](Request &req, Response &res)
          {
    const std::string id = req.param("id", "0");

    // In a real app you would look up DB here.
    // For showcase: return 404 when id == "0"
    if (id == "0")
    {
      res.status(404).json({"error", "User not found", "id", id});
      return;
    }

    res.json({
      "id", id,
      "name", "User#" + id,
      "vip", (id == "42")
    }); });

  // Multiple params
  app.get("/posts/{year}/{slug}", [](Request &req, Response &res)
          {
    const std::string year = req.param("year", "2026");
    const std::string slug = req.param("slug", "hello");

    res.json({
      "year", year,
      "slug", slug,
      "path", "/posts/" + year + "/" + slug
    }); });

  // A route that demonstrates “safe fallback”
  app.get("/files/{name}", [](Request &req, Response &res)
          {
    const std::string name = req.param("name", "unnamed");
    res.json({"file", name, "note", "This is only a routing showcase"}); });
}

static void register_query_routes(App &app)
{
  // Basic query param
  app.get("/search", [](Request &req, Response &res)
          {
    const std::string q = req.query_value("q", "");
    const std::string pageStr = req.query_value("page", "1");
    const std::string limitStr = req.query_value("limit", "10");

    const int page = to_int_or(pageStr, 1);
    const int limit = to_int_or(limitStr, 10);

    res.json({
      "q", q,
      "page", page,
      "limit", limit,
      "hint", "Try /search?q=vix&page=2&limit=5"
    }); });

  // Feature toggles via query
  app.get("/features", [](Request &req, Response &res)
          {
    const bool debug = to_bool(req.query_value("debug", "0"));
    const bool pretty = to_bool(req.query_value("pretty", "0"));

    res.json({
      "debug", debug,
      "pretty", pretty,
      "tip", "Try /features?debug=1&pretty=1"
    }); });

  // Sorting/pagination pattern
  app.get("/items", [](Request &req, Response &res)
          {
    const std::string sort = req.query_value("sort", "new");
    const int page = to_int_or(req.query_value("page", "1"), 1);
    const int limit = to_int_or(req.query_value("limit", "20"), 20);

    res.json({
      "sort", sort,
      "page", page,
      "limit", limit,
      "data", J::array({"item-1", "item-2", "item-3"})
    }); });
}

static void register_header_routes(App &app)
{
  // Echo some request headers
  app.get("/headers", [](Request &req, Response &res)
          {
    const std::string ua = req.header("User-Agent");
    const std::string accept = req.header("Accept");
    const std::string host = req.header("Host");

    res.json({
      "host", host,
      "user_agent", ua,
      "accept", accept
    }); });

  // Check for auth header presence
  app.get("/auth/check", [](Request &req, Response &res)
          {
    const bool hasAuth = req.has_header("Authorization");
    res.json({
      "has_authorization", hasAuth,
      "hint", "Send Authorization: Bearer <token>"
    }); });
}

static void register_body_routes(App &app)
{
  // Dump raw request body
  // (Useful for debugging when you call the route with curl)
  app.get("/echo/body", [](Request &req, Response &res)
          {
    const std::string body = req.body();
    res.json({
      "bytes", static_cast<long long>(body.size()),
      "body", body
    }); });

  // Parse JSON body (req.json()) and echo it back
  app.get("/echo/json", [](Request &req, Response &res)
          {
    const auto& j = req.json();
    // returns parsed JSON as response
    res.json(j); });

  // Read JSON fields safely (show the style; exact fields depend on your payload)
  app.get("/echo/json/fields", [](Request &req, Response &res)
          {
    const auto& j = req.json();

    // We keep it defensive: if fields don’t exist, we return defaults.
    std::string name = "unknown";
    bool vip = false;

    if (j.is_object())
    {
      if (j.contains("name") && j["name"].is_string()) name = j["name"].get<std::string>();
      if (j.contains("vip") && j["vip"].is_boolean()) vip = j["vip"].get<bool>();
    }

    res.json({
      "name", name,
      "vip", vip,
      "raw", j
    }); });
}

static void register_mixed_behavior_routes(App &app)
{
  // Using res + returning a value: the return is ignored if res already sent
  app.get("/mix", [](Request &, Response &res)
          {
    res.status(201).send("Created");
    return vix::json::o("ignored", true); });

  // Return a string (auto-send)
  app.get("/return/text", [](const Request &, Response &)
          { return "Auto-sent text payload"; });

  // Return a JSON object (auto-send)
  app.get("/return/json", [](const Request &, Response &)
          { return vix::json::o(
                "message", "Auto-sent JSON payload",
                "ok", true,
                "version", "vix"); });

  // Explicit res.json
  app.get("/send/json", [](Request &, Response &res)
          { res.json({"message", "Explicit res.json()", "ok", true}); });

  // Explicit res.send
  app.get("/send/text", [](Request &, Response &res)
          { res.send("Explicit res.send()"); });
}

static void register_demo_routes(App &app)
{
  // -------------------------------------------------------------------------
  // A big “demo” section: many tiny endpoints you can hit quickly.
  // This is intentionally verbose to represent real route variety.
  // -------------------------------------------------------------------------

  app.get("/demo/ping", [](Request &, Response &res)
          { res.json({"message", "pong"}); });

  app.get("/demo/version", [](Request &, Response &res)
          { res.json({"runtime", "Vix.cpp", "channel", "examples"}); });

  app.get("/demo/json/flat", [](Request &, Response &res)
          { res.json({"a", 1, "b", 2, "c", 3, "ok", true}); });

  app.get("/demo/json/nested", [](Request &, Response &res)
          { res.json({"user", J::obj({"id", 7, "name", "Ada", "roles", J::array({"admin", "dev"})}),
                      "meta", J::obj({"page", 1, "limit", 20})}); });

  app.get("/demo/params/{a}/{b}", [](Request &req, Response &res)
          {
    const std::string a = req.param("a", "x");
    const std::string b = req.param("b", "y");
    res.json({"a", a, "b", b, "concat", a + "-" + b}); });

  app.get("/demo/query", [](Request &req, Response &res)
          {
    const std::string a = req.query_value("a", "10");
    const std::string b = req.query_value("b", "32");
    const int ai = to_int_or(a, 10);
    const int bi = to_int_or(b, 32);
    res.json({"a", ai, "b", bi, "sum", ai + bi}); });

  app.get("/demo/ua", [](Request &req, Response &res)
          { res.json({"user_agent", req.header("User-Agent")}); });

  app.get("/demo/notfound", [](Request &, Response &res)
          { res.status(404).json({"error", "This is a demo 404"}); });

  app.get("/demo/created", [](Request &, Response &res)
          { res.status(201).json({"message", "Created sample"}); });

  app.get("/demo/accepted", [](Request &, Response &res)
          { res.status(202).json({"message", "Accepted sample"}); });

  app.get("/demo/no-content", [](Request &, Response &res)
          {
    // If your ResponseWrapper supports no-content behavior, it will handle it.
    // Here we keep it simple: send JSON with 204 is typically empty,
    // but for showcase we just send a 204 without payload.
    res.set_status(204);
    res.send(); });

  // A small “router-style” layout
  app.get("/demo/products", [](Request &, Response &res)
          { res.json({"data", J::array({J::obj({"id", 1, "title", "Phone", "price", 120}),
                                        J::obj({"id", 2, "title", "Laptop", "price", 900}),
                                        J::obj({"id", 3, "title", "Headphones", "price", 35})})}); });

  app.get("/demo/products/{id}", [](Request &req, Response &res)
          {
    const std::string id = req.param("id", "0");
    if (id == "0")
    {
      res.status(404).json({"error", "Product not found"});
      return;
    }
    res.json({"id", id, "title", "Product#" + id, "price", 123}); });

  // “Auth-like” sample
  app.get("/demo/me", [](Request &req, Response &res)
          {
    const std::string token = req.header("Authorization");
    if (token.empty())
    {
      res.status(401).json({"error", "Missing Authorization header"});
      return;
    }
    res.json({"id", 1, "name", "Ada", "token_received", true}); });

  // “Validation-like” sample
  app.get("/demo/validate", [](Request &req, Response &res)
          {
    const std::string email = req.query_value("email", "");
    if (email.find('@') == std::string::npos)
    {
      res.status(400).json({"error", "Invalid email", "field", "email"});
      return;
    }
    res.json({"ok", true, "email", email}); });

  // -------------------------------------------------------------------------
  // A LOT of tiny endpoints that demonstrate consistent style.
  // (Useful when people scroll the file: it looks like a real framework.)
  // -------------------------------------------------------------------------

  app.get("/demo/1", [](Request &, Response &res)
          { res.json({"route", 1, "ok", true}); });
  app.get("/demo/2", [](Request &, Response &res)
          { res.json({"route", 2, "ok", true}); });
  app.get("/demo/3", [](Request &, Response &res)
          { res.json({"route", 3, "ok", true}); });
  app.get("/demo/4", [](Request &, Response &res)
          { res.json({"route", 4, "ok", true}); });
  app.get("/demo/5", [](Request &, Response &res)
          { res.json({"route", 5, "ok", true}); });
  app.get("/demo/6", [](Request &, Response &res)
          { res.json({"route", 6, "ok", true}); });
  app.get("/demo/7", [](Request &, Response &res)
          { res.json({"route", 7, "ok", true}); });
  app.get("/demo/8", [](Request &, Response &res)
          { res.json({"route", 8, "ok", true}); });
  app.get("/demo/9", [](Request &, Response &res)
          { res.json({"route", 9, "ok", true}); });
  app.get("/demo/10", [](Request &, Response &res)
          { res.json({"route", 10, "ok", true}); });

  app.get("/demo/json/a", [](Request &, Response &res)
          { res.json({"a", 1, "b", 2, "c", 3}); });

  app.get("/demo/json/b", [](Request &, Response &res)
          { res.json({"numbers", J::array({1, 2, 3, 4, 5}),
                      "flags", J::array({true, false, true})}); });

  app.get("/demo/json/c", [](Request &, Response &res)
          { res.json({"obj", J::obj({"x", 10,
                                     "y", 20,
                                     "inner", J::obj({"ok", true})})}); });

  app.get("/demo/path/{x}", [](Request &req, Response &res)
          { res.json({"x", req.param("x", "none")}); });

  app.get("/demo/path/{x}/{y}", [](Request &req, Response &res)
          { res.json({"x", req.param("x", "none"), "y", req.param("y", "none")}); });

  app.get("/demo/query/required", [](Request &req, Response &res)
          {
    const std::string q = req.query_value("q", "");
    if (q.empty())
    {
      res.status(400).json({"error", "Missing query param", "param", "q"});
      return;
    }
    res.json({"q", q, "ok", true}); });

  app.get("/demo/query/flags", [](Request &req, Response &res)
          {
    const bool a = to_bool(req.query_value("a", "0"));
    const bool b = to_bool(req.query_value("b", "0"));
    res.json({"a", a, "b", b, "both", (a && b)}); });

  app.get("/demo/headers/accept", [](Request &req, Response &res)
          { res.json({"accept", req.header("Accept")}); });

  app.get("/demo/headers/lang", [](Request &req, Response &res)
          { res.json({"accept_language", req.header("Accept-Language")}); });

  app.get("/demo/headers/encoding", [](Request &req, Response &res)
          { res.json({"accept_encoding", req.header("Accept-Encoding")}); });
}

static void register_all_routes(App &app)
{
  register_basic_routes(app);
  register_status_routes(app);
  register_path_param_routes(app);
  register_query_routes(app);
  register_header_routes(app);
  register_body_routes(app);
  register_mixed_behavior_routes(app);
  register_demo_routes(app);
}

// ---------------------------------------------------------------------------
// Bootstrap
// ---------------------------------------------------------------------------

static int run_server()
{
  App app;
  register_all_routes(app);

  // Port
  app.run(8080);
  return 0;
}

int main()
{
  return run_server();
}
