// ============================================================================
// json_builders_routes.cpp — Minimal routes using Vix::json builders
// GET /hello          -> {"message":"Hello, World!"}
// GET /users/{id}     -> {"user":{"id":"<id>","active":true}}
// GET /roles          -> {"roles":["admin","editor","viewer"]}
// ============================================================================

#include <vix/core.h>
#include <vix/json/json.hpp>
#include <string>

namespace J = Vix::json;

int main()
{
        Vix::App app;

        // GET /hello -> {"message": "Hello, World!"}
        app.get("/hello", [](auto & /*req*/, auto &res)
                { res.json(J::o("message", "Hello, World!")); });

        // GET /users/{id} -> {"user": {"id": "...", "active": true}}
        app.get("/users/{id}", [](auto & /*req*/, auto &res, auto &params)
                {
        const std::string id = params["id"];
        res.json(J::o("user", J::o("id", id, "active", true))); });

        // GET /roles -> {"roles": ["admin", "editor", "viewer"]}
        app.get("/roles", [](auto & /*req*/, auto &res)
                { res.json(J::o("roles", J::a("admin", "editor", "viewer"))); });

        app.run(8080);
        return 0;
}
