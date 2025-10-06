#include <vix/core.h>

int main()
{
    Vix::App app;

    app.del("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        res.json({{"message", "User deleted"}, {"id", id}}); });

    app.run(8080);
}
