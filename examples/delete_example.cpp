// DELETE example — delete a resource by id
#include <vix/core.h>
#include <nlohmann/json.hpp>
#include <string>

int main()
{
    Vix::App app;

    // Delete user
    app.del("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        const std::string id = params["id"];

        // In a real app you'd remove from DB/store here
        res.json(nlohmann::json{
            {"action", "delete"},
            {"status", "deleted"},
            {"user_id", id}
        }); });

    app.run(8080);
    return 0;
}
