// POST example — create a resource from JSON body
#include <vix/core.h>
#include <nlohmann/json.hpp>
#include <string>

int main()
{
    Vix::App app;

    // Create user
    app.post("/users", [](auto &req, auto &res)
             {
        try {
            auto body = nlohmann::json::parse(req.body());

            // echo minimal "created" payload
            nlohmann::json out = {
                {"action", "create"},
                {"status", "created"},
                {"user", {
                    {"name",  body.value("name",  "")},
                    {"email", body.value("email", "")},
                    {"age",   body.value("age",   0)}
                }}
            };
            res.status(Vix::http::status::created).json(out);
        } catch (...) {
            res.status(Vix::http::status::bad_request)
               .json(nlohmann::json{{"error", "Invalid JSON"}});
        } });

    app.run(8080);
    return 0;
}
