// ============================================================================
// json_builders_routes.cpp — Minimal routes using Vix::json builders (new API)
// GET /hello          -> {"message":"Hello, World!"}
// GET /users/{id}     -> {"user":{"id":"<id>","active":true}}
// GET /roles          -> {"roles":["admin","editor","viewer"]}
// ============================================================================

#include <vix.hpp>
#include <vix/json/Simple.hpp>

using namespace vix;
namespace J = vix::json;

int main()
{
    App app;

    // GET /hello -> {"message": "Hello, World!"}
    app.get("/hello", [](Request &, Response &res)
            { res.json({"message", "Hello, World!"}); });

    // GET /users/{id} -> {"user": {"id": "...", "active": true}}
    app.get("/users/{id}", [](Request &req, Response &res)
            {
        const std::string id = req.param("id");
        res.json({
            "user", J::obj({
                "id",     id,
                "active", true
            })
        }); });

    // GET /roles -> {"roles": ["admin", "editor", "viewer"]}
    app.get("/roles", [](Request &, Response &res)
            { res.json({"roles", J::array({"admin", "editor", "viewer"})}); });

    app.run(8080);
    return 0;
}
