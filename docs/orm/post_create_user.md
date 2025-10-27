# Example — post_create_user.cpp

```cpp
#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace Vix;
namespace J = Vix::json;

int main()
{
    App app;

    // POST /users
    app.post("/users", [](auto &req, auto &res)
             {
        try {
            // Parse body as nlohmann::json for simplicity (still supported)
            auto body = nlohmann::json::parse(req.body());

            const std::string name  = body.value("name",  "");
            const std::string email = body.value("email", "");
            const int age           = body.value("age",   0);

            res.status(http::status::created).json({
                "action", "create",
                "status", "created",
                "user", J::obj({
                    "name",  name,
                    "email", email,
                    "age",   static_cast<long long>(age)
                })
            });
        }
        catch (...) {
            res.status(http::status::bad_request).json({
                "error", "Invalid JSON"
            });
        } });

    app.run(8080);
}
```
