// ============================================================================
// put_update_user.cpp — PUT example (new Vix.cpp API)
// PUT /users/{id} -> {"action":"update","status":"updated","user":{...}}
// ============================================================================

#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace vix;
namespace J = vix::json;

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
            res.status(400).json({
                "error", "Invalid JSON"
            });
        } });

    app.run(8080);
    return 0;
}
