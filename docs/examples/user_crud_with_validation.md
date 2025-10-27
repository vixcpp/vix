# Example — user_crud_with_validation

```cpp
// ============================================================================
// user_crud_with_validation.cpp — Full CRUD + Validation (Vix.cpp, nouvelle API)
// ----------------------------------------------------------------------------
// Routes:
//   POST   /users          → Create user (with validation)
//   GET    /users/{id}     → Read user
//   PUT    /users/{id}     → Update user
//   DELETE /users/{id}     → Delete user
// ============================================================================

#include <vix.hpp>                  // App, http, ResponseWrapper, etc.
#include <vix/json/Simple.hpp>      // Vix::json::token, obj(), array()
#include <vix/utils/Validation.hpp> // required(), num_range(), match(), validate_map
#include <nlohmann/json.hpp>

#include <unordered_map>
#include <mutex>
#include <string>
#include <optional>
#include <sstream>
#include <vector>

using namespace Vix;
namespace J = Vix::json;
using njson = nlohmann::json;
using namespace Vix::utils;

// --------------------------- Data Model -------------------------------------
struct User
{
    std::string id;
    std::string name;
    std::string email;
    int age{};
};

static std::mutex g_mtx;
static std::unordered_map<std::string, User> g_users;

// --------------------------- Helpers ----------------------------------------
static J::kvs user_to_kvs(const User &u)
{
    return J::obj({"id", u.id,
                   "name", u.name,
                   "email", u.email,
                   "age", static_cast<long long>(u.age)});
}

static std::string to_string_safe(const njson &j)
{
    if (j.is_string())
        return j.get<std::string>();
    if (j.is_number_integer())
        return std::to_string(j.get<long long>());
    if (j.is_number_unsigned())
        return std::to_string(j.get<unsigned long long>());
    if (j.is_number_float())
        return std::to_string(j.get<double>());
    if (j.is_boolean())
        return j.get<bool>() ? "true" : "false";
    return {};
}

static bool parse_user(const njson &j, User &out)
{
    try
    {
        out.name = j.value("name", std::string{});
        out.email = j.value("email", std::string{});

        if (j.contains("age"))
        {
            if (j["age"].is_string())
                out.age = std::stoi(j["age"].get<std::string>());
            else if (j["age"].is_number_integer())
                out.age = static_cast<int>(j["age"].get<long long>());
            else if (j["age"].is_number_unsigned())
                out.age = static_cast<int>(j["age"].get<unsigned long long>());
            else if (j["age"].is_number_float())
                out.age = static_cast<int>(j["age"].get<double>());
            else
                out.age = 0;
        }
        else
        {
            out.age = 0;
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

static std::string gen_id_from_email(const std::string &email)
{
    const auto h = std::hash<std::string>{}(email) & 0xFFFFFFull;
    std::ostringstream oss;
    oss << h;
    return oss.str();
}

// --------------------------- Main -------------------------------------------
int main()
{
    App app;

    // CREATE (POST /users)
    app.post("/users", [](auto &req, auto &res)
             {
        njson body;
        try {
            body = njson::parse(req.body());
        } catch (...) {
            res.status(http::status::bad_request).json({
                "error", "Invalid JSON"
            });
            return;
        }

        // Prépare les champs pour la validation (map<string,string>)
        std::unordered_map<std::string, std::string> data{
            {"name",  body.value("name",  std::string{})},
            {"email", body.value("email", std::string{})},
            {"age",   body.contains("age") ? to_string_safe(body["age"]) : std::string{}}
        };

        Schema sch{
            {"name",  required("name")},
            {"age",   num_range(1, 150, "Age")},
            {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Invalid email")}
        };

        auto r = validate_map(data, sch);
        if (r.is_err()) {
            // Construire {"errors": { field: message, ... }} SANS nlohmann
            std::vector<J::token> flat;
            flat.reserve(r.error().size() * 2);
            for (const auto& kv : r.error()) {
                flat.emplace_back(kv.first);   // clé
                flat.emplace_back(kv.second);  // valeur
            }

            res.status(http::status::bad_request).json({
                "errors", J::obj(std::move(flat))
            });
            return;
        }

        User u;
        if (!parse_user(body, u)) {
            res.status(http::status::bad_request).json({
                "error", "Invalid fields"
            });
            return;
        }

        u.id = gen_id_from_email(u.email);

        {
            std::lock_guard<std::mutex> lock(g_mtx);
            g_users[u.id] = u;
        }

        res.status(http::status::created).json({
            "status", "created",
            "user",   user_to_kvs(u)
        }); });

    // READ (GET /users/{id})
    app.get("/users/{id}", [](auto & /*req*/, auto &res, auto &params)
            {
        const std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end()) {
            res.status(http::status::not_found).json({
                "error", "User not found"
            });
            return;
        }
        res.json({
            "user", user_to_kvs(it->second)
        }); });

    // UPDATE (PUT /users/{id})
    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        const std::string id = params["id"];

        njson body;
        try {
            body = njson::parse(req.body());
        } catch (...) {
            res.status(http::status::bad_request).json({
                "error", "Invalid JSON"
            });
            return;
        }

        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end()) {
            res.status(http::status::not_found).json({
                "error", "User not found"
            });
            return;
        }

        if (body.contains("name"))  it->second.name  = body["name"].get<std::string>();
        if (body.contains("email")) it->second.email = body["email"].get<std::string>();
        if (body.contains("age")) {
            if      (body["age"].is_string())          it->second.age = std::stoi(body["age"].get<std::string>());
            else if (body["age"].is_number_integer())  it->second.age = static_cast<int>(body["age"].get<long long>());
            else if (body["age"].is_number_unsigned()) it->second.age = static_cast<int>(body["age"].get<unsigned long long>());
            else if (body["age"].is_number_float())    it->second.age = static_cast<int>(body["age"].get<double>());
        }

        res.json({
            "status", "updated",
            "user",   user_to_kvs(it->second)
        }); });

    // DELETE (DELETE /users/{id})
    app.del("/users/{id}", [](auto & /*req*/, auto &res, auto &params)
            {
        const std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        const auto n = g_users.erase(id);
        if (!n) {
            res.status(http::status::not_found).json({
                "error", "User not found"
            });
            return;
        }
        res.json({
            "status",  "deleted",
            "user_id", id
        }); });

    // Lancement
    app.run(8080);
    return 0;
}


```
