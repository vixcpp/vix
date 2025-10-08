// ============================================================================
// main.cpp — Quick Example for Vix.cpp
// ----------------------------------------------------------------------------
// Minimal REST API demonstrating the simplicity of the Vix framework.
// ----------------------------------------------------------------------------
// Routes:
//   GET  /hello         → returns a JSON greeting
//   GET  /users/{id}    → returns a user ID as JSON
// ============================================================================

#include <vix/core.h>
#include <nlohmann/json.hpp>

int main()
{
        Vix::App app;

        // Simple JSON route
        app.get("/hello", [](auto &req, auto &res)
                { res.json(nlohmann::json{{"message", "Hello, Vix!"}}); });

        // Example with path parameter
        app.get("/users/{id}", [](auto &req, auto &res, auto &params)
                { res.json(nlohmann::json{
                      {"user_id", params["id"]},
                      {"framework", "Vix.cpp"}}); });

        app.run(8080);
        return 0;
}
