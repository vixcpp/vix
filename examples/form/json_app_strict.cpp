// ============================================================================
// json_app_strict.cpp — JSON parser (App) strict example (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run json_app_strict.cpp
//
// Test:
//   # OK
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: application/json; charset=utf-8" \
//     --data '{"x":1}'
//
//   # Empty body -> 400 (empty_body)
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: application/json" \
//     --data ""
//
//   # Invalid JSON -> 400 (invalid_json)
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: application/json" \
//     --data '{"x":}'
//
//   # Invalid content-type -> 415
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: text/plain" \
//     --data '{"x":1}'
//
//   # Payload too large -> 413 (max_bytes demo)
//   BIG="$(python3 -c 'print("{\"x\":\"" + "a"*300 + "\"}")')"
//   curl -i -X POST http://localhost:8080/json \
//     -H "Content-Type: application/json" \
//     --data "$BIG"
// ============================================================================
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
    App app;

    // STRICT JSON: Content-Type required + body required (allow_empty=false)
    app.use("/json", middleware::app::json_dev(
                         /*max_bytes=*/256,
                         /*allow_empty=*/false,
                         /*require_content_type=*/true));

    app.get("/", [](Request &, Response &res)
            { res.send("POST /json requires a non-empty JSON body."); });

    app.post("/json", [](Request &req, Response &res)
             {
                 auto &jb = req.state<middleware::parsers::JsonBody>();

                 if (jb.value.contains("x"))
                     res.status(200).send(jb.value["x"].dump());
                 else
                     res.status(200).send(jb.value.dump()); });

    app.run(8080);
    return 0;
}
