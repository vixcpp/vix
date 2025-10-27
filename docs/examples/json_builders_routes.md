# Example — json_builders_routes

```cpp
#include <vix.hpp>
#include <vix/json/Simple.hpp>

using namespace Vix;
namespace J = Vix::json;

int main()
{
    App app;

    // GET /hello -> {"message": "Hello, World!"}
    app.get("/hello", [](auto &, auto &res)
            { res.json({"message", "Hello, World!"}); });

    // GET /users/{id} -> {"user": {"id": "...", "active": true}}
    app.get("/users/{id}", [](auto &, auto &res, auto &params)
            {
    const std::string id = params["id"];
    res.json({
        "user", J::obj({
            "id",     id,
            "active", true
        })
    }); });

    // GET /roles -> {"roles": ["admin", "editor", "viewer"]}
    app.get("/roles", [](auto &, auto &res)
            { res.json({"roles", J::array({"admin", "editor", "viewer"})}); });

    app.run(8080);
    return 0;
}
```
