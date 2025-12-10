//
// examples/http/json_api.cpp
//
// Simple JSON API example using Vix.cpp.
//
// Demonstrates:
//   - A small "users" API under /api/users
//   - GET /api/users        → returns a JSON array
//   - POST /api/users       → accepts a JSON body and echoes basic info
//
// Note:
//   This example uses nlohmann::json for parsing the request body.
//   Make sure nlohmann/json.hpp is available in your include path.
//

#include <vix.hpp>
#include <nlohmann/json.hpp>

using namespace vix;

int main()
{
    App app;

    // In-memory fake "database"
    std::vector<nlohmann::json> users = {
        {{"id", 1}, {"name", "Alice"}, {"role", "admin"}},
        {{"id", 2}, {"name", "Bob"}, {"role", "user"}}};

    // --------------------------------------------------------
    // GET /api/users
    //
    // Returns the full list of users as JSON.
    // --------------------------------------------------------
    app.get("/api/users", [&users](auto &, auto &res)
            {
        json payload = {
            {"count", users.size()},
            {"items", users}
        };

        res.json(payload); });

    // --------------------------------------------------------
    // POST /api/users
    //
    // Expects a JSON body like:
    //   { "name": "Charlie", "role": "user" }
    //
    // For simplicity, we:
    //   - parse the body
    //   - assign a new incremental id
    //   - push it into the in-memory vector
    //   - return the created user
    // --------------------------------------------------------
    app.post("/api/users", [&users](auto &req, auto &res)
             {
        try
        {
            // req is typically a boost::beast::http::request<string_body>
            const auto& body = req.body();
            auto data = json::parse(body);

            // Generate a very simple new id
            int newId = users.empty()
                        ? 1
                        : (users.back().value("id", 0) + 1);

            json user = {
                {"id",   newId},
                {"name", data.value("name", "unknown")},
                {"role", data.value("role", "user")}
            };

            users.push_back(user);

            res.status(201).json({
                {"message", "User created"},
                {"user",    user}
            });
        }
        catch (const std::exception& e)
        {
            res.status(400).json({
                {"error",   "Invalid JSON payload"},
                {"details", e.what()}
            });
        } });

    // --------------------------------------------------------
    // Root route: hint for trying the API
    // --------------------------------------------------------
    app.get("/", [](auto &, auto &res)
            { res.json({"api", "/api/users",
                        "hint1", "GET /api/users",
                        "hint2", "POST /api/users with JSON body {\"name\":\"Charlie\",\"role\":\"user\"}"}); });

    app.run(8080);
    return 0;
}
