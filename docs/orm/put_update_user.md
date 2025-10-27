# Example — put_update_user.cpp

```cpp
#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace Vix;
namespace J = Vix::json;

int main()
{
    App app;

    // PUT /users/{id}
    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        const std::string id = params["id"];

        try {
            // Parsing with nlohmann::json for input is fine (Vix supports it internally)
            auto body = nlohmann::json::parse(req.body());

            const std::string name  = body.value("name",  "");
            const std::string email = body.value("email", "");
            const int age           = body.value("age",   0);

            res.json({
                "action", "update",
                "status", "updated",
                "user", J::obj({
                    "id",    id,
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
