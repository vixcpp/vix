// ============================================================================
// body_limit_app.cpp — Body limit middleware example (Vix.cpp)
//
// Run:
//   vix run vix/examples/body_limit_app.cpp
//
// Tests:
//
// 1) Small body (OK)
//   curl -i -X POST http://localhost:8080/api/echo \
//     -H "Content-Type: text/plain" \
//     --data "hello"
//
// 2) Large body (413 Payload Too Large)
//   python3 - <<'PY'
// import requests
// print(requests.post("http://localhost:8080/api/echo", data="x"*64).status_code)
// PY
//
// 3) GET ignored by default (apply_to_get=false)
//   curl -i http://localhost:8080/api/ping
//
// 4) Strict mode: require Content-Length (411) (see route /api/strict)
//   curl -i -X POST http://localhost:8080/api/strict \
//     -H "Transfer-Encoding: chunked" \
//     -H "Content-Type: text/plain" \
//     --data "hello"
//
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static void register_routes(App &app)
{
    app.get("/", [](Request &, Response &res)
            { res.send("body_limit example: /api/ping, /api/echo, /api/strict"); });

    app.get("/api/ping", [](Request &, Response &res)
            { res.json({"ok", true, "msg", "pong"}); });

    app.post("/api/echo", [](Request &req, Response &res)
             { res.json({"ok", true,
                         "bytes", static_cast<long long>(req.body().size()),
                         "content_type", req.header("content-type")}); });

    app.post("/api/strict", [](Request &req, Response &res)
             { res.json({"ok", true,
                         "msg", "strict accepted",
                         "bytes", static_cast<long long>(req.body().size())}); });
}

int main()
{
    App app;

    // /api: max 32 bytes (demo), chunked allowed
    app.use("/api", middleware::app::body_limit_dev(
                        32,    // max_bytes
                        false, // apply_to_get
                        true   // allow_chunked
                        ));

    // /api/strict: max 32 bytes, chunked NOT allowed => 411 if missing Content-Length
    app.use("/api/strict", middleware::app::body_limit_dev(
                               32,    // max_bytes
                               false, // apply_to_get
                               false  // allow_chunked (strict)
                               ));

    register_routes(app);

    app.run(8080);
    return 0;
}
