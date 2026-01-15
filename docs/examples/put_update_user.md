# Example — put_update_user.cpp

```cpp
#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace vix;
namespace J = vix::json;

int main()
{
    App app;

    // PUT /users/{id}
    app.put("/users/{id}", [](Request &req, Response &res)
            {
        const std::string id = req.param("id");

        try {
            auto body = json::Json::parse(req.body());

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
            res.status(400).json({
                "error", "Invalid JSON"
            });
        } });

    app.run(8080);
}
```
