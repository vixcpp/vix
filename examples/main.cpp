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

        // GET /
        app.get("/", [](auto &, auto &res)
                { res.json({"framework", "Vix.cpp",
                            "message", "Welcome to the future of C++ web development 🚀"}); });

        // GET /hello/{name}
        app.get("/hello/{name}", [](auto &, auto &res, auto &params)
                { res.json({"greeting", "Hello " + params["name"] + " 👋",
                            "powered_by", "Vix.cpp"}); });

        // Start server
        app.run(8080);
        return 0;
}
