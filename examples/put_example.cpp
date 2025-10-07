#include <vix/core.h>
#include <nlohmann/json.hpp>

int main()
{
    Vix::App app;

    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        std::string name = "Jane";
        res.json(nlohmann::json{{"message", "User updated"}, {"id", id}, {"name", name}}); });

    app.run(8080);
    return 0;
}
