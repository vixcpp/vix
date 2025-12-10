// ============================================================================
// main.cpp — Quick Start Example (Vix.cpp)
// ---------------------------------------------------------------------------
// A minimal HTTP server built with Vix.cpp.
// Run →  ./main
// Then visit →  http://localhost:8080/hello
// ============================================================================
#include <vix.hpp>

using namespace vix;

int main()
{
    App app;

    app.get("/users/{id}", [](Request &req, Response &res)
            {
        auto id   = req.param("id");
        auto page = req.query_value("page", "1");

        res.json({
            "framework", "Vix.cpp",
            "id",        id,
            "page",      page
        }); });

    app.post("/echo/{id}", [](Request &req, Response &res)
             {
    const auto &body = req.json();

    res.json({
        "ok",    true,
        "echo",  body,
        "query", req.query(),
        "params", req.params()
    }); });

    app.run(8080);
}
