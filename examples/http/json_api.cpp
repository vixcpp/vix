/**
 *
 *  @file examples/http/json_api.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  // In-memory fake "database"
  std::vector<json::Json> users = {
      {{"id", 1}, {"name", "Alice"}, {"role", "admin"}},
      {{"id", 2}, {"name", "Bob"}, {"role", "user"}}};

  // GET /api/users
  //
  // Returns the full list of users as JSON.
  app.get("/api/users", [&users](Request &, Response &res)
          {
        json::Json payload = {
            {"count", users.size()},
            {"items", users}
        };

        res.json(payload); });

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
  app.post("/api/users", [&users](Request &req, Response &res)
           {
        try
        {
            // req is typically a boost::beast::http::request<string_body>
            const auto& body = req.body();
            json::Json data = json::Json::parse(body);

            // Generate a very simple new id
            int newId = users.empty()
                        ? 1
                        : (users.back().value("id", 0) + 1);

            json::Json user = json::Json({
                {"id",   newId},
                {"name", data.value("name", "unknown")},
                {"role", data.value("role", "user")}
            });

            users.push_back(user);

            res.status(201).send(vix::json::kv({
                {"message", "User created"},
                {"user",    user}
            }));
        }
        catch (const std::exception& e)
        {
            res.status(400).send(vix::json::kv({
                {"error",   "Invalid JSON payload"},
                {"details", e.what()}
            }));
        } });

  // Root route: hint for trying the API
  app.get("/", [](Request &, Response &res)
          { res.json({"api", "/api/users",
                      "hint1", "GET /api/users",
                      "hint2", "POST /api/users with JSON body {\"name\":\"Charlie\",\"role\":\"user\"}"}); });

  app.run(8080);
  return 0;
}
