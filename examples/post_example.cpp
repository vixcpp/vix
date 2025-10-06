#include <vix/core.h>

int main()
{
    Vix::App app;

    app.post("/users", [](auto &req, auto &res)
             {
        auto body = nlohmann::json::parse(req.body());
        std::string name = body.value("name", "unknown");
        res.json({{"message", "User created"}, {"name", name}}); });

    app.run(8080);
}
