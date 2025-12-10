//
// examples/http/basic_get.cpp
//
// Minimal HTTP server using Vix.cpp.
//
// Demonstrates:
//   - Creating a vix::App
//   - Registering a simple GET route
//   - Returning a JSON response
//

#include <vix.hpp>

using namespace vix;

int main()
{
    App app;

    // GET /
    app.get("/", [](auto &, auto &res)
            { res.json({"framework", "Vix.cpp",
                        "message", "Hello from basic_get.cpp 👋"}); });

    // Start the HTTP server on port 8080
    app.run(8080);
    return 0;
}
