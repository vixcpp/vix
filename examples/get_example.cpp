// GET example — simple read-only endpoints
#include <vix/core.h>
#include <nlohmann/json.hpp>
#include <string>

int main()
{
    Vix::App app;

    // Simple health/hello
    app.get("/hello", [](auto &req, auto &res)
            { res.json(nlohmann::json{{"message", "Hello, World!"}}); });

    // Read with path parameter
    app.get("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        res.json(nlohmann::json{
            {"action", "read"},
            {"user_id", id}
        }); });

    app.run(8080);
    return 0;
}
