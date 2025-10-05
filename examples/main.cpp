#include <vix/core.h>

int main()
{
    Vix::App app;

    app.get("/hello", [](auto &req, auto &res)
            { res.json({{"message", "Hello, World!"}}); });

    app.get("/ping", [](auto &req, auto &res)
            { res.text("pong"); });

    app.get("/users/{id}", [](auto &req, auto &res, auto &params)
            {
    std::string id = params["id"]; 
    res.json({{"user_id", id}}); });

    app.run(8080);
}
