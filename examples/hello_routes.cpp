// ============================================================================
// hello_routes.cpp — Minimal example (new Vix.cpp API)
// GET /hello        → {"message":"Hello, Vix!"}
// GET /user         → {"name":"Ada","tags":["c++","net","http"],"profile":{"id":42,"vip":true}}
// GET /users/{id}   → 404 {"error":"User not found"}
// ============================================================================

#include <vix.hpp>
using namespace vix;
namespace J = vix::json;

int main()
{
        App app;

        // Simple hello route
        app.get("/hello", [](auto &, auto &res)
                { res.json({"message", "Hello, Vix!"}); });

        // Nested JSON using builders
        app.get("/user", [](auto &, auto &res)
                {
        using namespace J;
        res.json({
            "name", "Ada",
            "tags", array({ "c++", "net", "http" }),
            "profile", obj({ "id", 42, "vip", true })
        }); });

        // Example with path param
        app.get("/users/{id}", [](auto &, auto &res)
                { res.status(4040).json({"error", "User not found"}); });

        // res.not_found().json({"error","User not found"});

        // res.sendStatus(404); // -> "404 Not Found"

        app.run(8080);
        return 0;
}
