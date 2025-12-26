// ============================================================================
// multipart_app_simple.cpp — Multipart parser (App) simple example (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run multipart_app_simple.cpp
//
// Test:
//   # OK (multipart with boundary)
//   curl -i -X POST "http://localhost:8080/mp" \
//     -F "a=1" -F "b=hello"
//
//   # Missing boundary (force a raw header without boundary) -> 400
//   curl -i -X POST "http://localhost:8080/mp" \
//     -H "Content-Type: multipart/form-data" \
//     --data "x"
//
//   # Invalid content-type -> 415
//   curl -i -X POST "http://localhost:8080/mp" \
//     -H "Content-Type: text/plain" \
//     --data "x"
//
//   # Payload too large -> 413 (max_bytes demo)
//   BIG="$(python3 -c 'print(\"x\"*300)')"
//   curl -i -X POST "http://localhost:8080/mp" \
//     -H "Content-Type: multipart/form-data; boundary=----X" \
//     --data "----X\r\n${BIG}\r\n----X--\r\n"
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
    App app;

    app.use("/mp", middleware::app::cors_dev());
    app.use("/mp", middleware::app::multipart_save_dev("uploads"));

    app.options("/mp", [](Request &, Response &res)
                { res.status(204).send(""); });

    app.get("/", [](Request &, Response &res)
            { res.send("POST /mp multipart/form-data (saves files to ./uploads/)"); });

    app.post("/mp", [](Request &req, Response &res)
             {
                 auto &form = req.state<middleware::parsers::MultipartForm>();
                 res.json(middleware::app::multipart_json(form)); });

    app.run(8080);
    return 0;
}