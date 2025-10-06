#include <vix/core.h>

int main()
{
    Vix::App app;

    // Simple GET route
    app.get("/hello", [](auto &req, auto &res)
            { res.json({{"message", "Hello, World!"}}); });

    // GET route with path parameter
    app.get("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        res.json({{"user_id", id}}); });

    app.run(8080);
}
