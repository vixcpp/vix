// ============================================================================
// form_app_simple.cpp — Form parser (App) simple example (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run form_app_simple.cpp
//
// Test:
//   # OK
//   curl -i -X POST "http://localhost:8080/form" \
//     -H "Content-Type: application/x-www-form-urlencoded" \
//     --data "a=1&b=hello+world"
//
//   # Missing/invalid content-type -> 415
//   curl -i -X POST "http://localhost:8080/form" \
//     -H "Content-Type: text/plain" \
//     --data "a=1&b=hello+world"
//
//   # Payload too large -> 413 (max_bytes demo)
//   curl -i -X POST "http://localhost:8080/form" \
//     -H "Content-Type: application/x-www-form-urlencoded" \
//     --data "$(python3 - <<'PY'\nprint('a=' + 'x'*200)\nPY)"
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
    App app;

    app.use("/form", middleware::app::form_dev(128));

    app.get("/", [](Request &, Response &res)
            { res.send("POST /form (application/x-www-form-urlencoded)"); });

    app.post("/form", [](Request &req, Response &res)
             {
        auto& fb = req.state<middleware::parsers::FormBody>();

        auto it = fb.fields.find("b");
        res.send(it == fb.fields.end() ? "" : it->second); });

    app.run(8080);
    return 0;
}
