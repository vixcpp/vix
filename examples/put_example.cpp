// PUT example — update a resource by id
#include <vix/core.h>
#include <nlohmann/json.hpp>
#include <string>

int main()
{
    Vix::App app;

    // Update user
    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        const std::string id = params["id"];
        try {
            auto body = nlohmann::json::parse(req.body());

            // echo minimal "updated" payload
            nlohmann::json out = {
                {"action", "update"},
                {"status", "updated"},
                {"user", {
                    {"id",    id},
                    {"name",  body.value("name",  nlohmann::json())},
                    {"email", body.value("email", nlohmann::json())},
                    {"age",   body.value("age",   nlohmann::json())}
                }}
            };
            res.json(out);
        } catch (...) {
            res.status(Vix::http::status::bad_request)
               .json(nlohmann::json{{"error", "Invalid JSON"}});
        } });

    app.run(8080);
    return 0;
}
