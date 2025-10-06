#include <vix/core.h>

int main()
{
    Vix::App app;

    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        auto body = nlohmann::json::parse(req.body());
        std::string name = body.value("name", "unknown");
        res.json({{"message", "User updated"}, {"id", id}, {"name", name}}); });

    app.run(8080);
}
