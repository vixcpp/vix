#include <vix/core.h>
#include <nlohmann/json.hpp>

int main()
{
        Vix::App app;

        app.get("/hello", [](auto &req, auto &res)
                { res.json(nlohmann::json{{"message", "Hello, World!"}}); });

        app.get("/users/{id}", [](auto &req, auto &res, auto &params)
                {
        std::string id = params["id"];
        res.json(nlohmann::json{{"user_id", id}}); });

        app.run(8080);
        return 0;
}
