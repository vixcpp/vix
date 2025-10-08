// ============================================================================
// full_crud_with_validation.cpp — Full CRUD + Validation (Vix.cpp)
// ----------------------------------------------------------------------------
// Complete demonstration of CRUD operations using Vix::App, with validation
// (Vix::utils::Validation) and JSON serialization (Vix::json).
//
// Routes:
//   POST   /users          → Create user (with validation)
//   GET    /users/{id}     → Read user
//   PUT    /users/{id}     → Update user
//   DELETE /users/{id}     → Delete user
//
// Thread-safe: std::mutex protects shared user map
// ============================================================================

#include <vix/core.h>
#include <vix/json/json.hpp>
#include <vix/utils/Validation.hpp>

#include <unordered_map>
#include <mutex>
#include <string>

namespace J = Vix::json;
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

// --------------------------- JSON Helpers -----------------------------------
static J::Json to_json(const User &u)
{
    return J::o("id", u.id, "name", u.name, "email", u.email, "age", u.age);
}

static bool parse_user(const J::Json &j, User &out)
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

// --------------------------- Main Application -------------------------------
int main()
{
    Vix::App app;

    // CREATE (POST /users)
    app.post("/users", [](auto &req, auto &res)
             {
        J::Json body;
        try { body = J::loads(req.body()); }
        catch (...) {
            res.status(Vix::http::status::bad_request)
               .json(J::o("error", "Invalid JSON"));
            return;
        }

        std::unordered_map<std::string, std::string> data{
            {"name",  body.value("name",  std::string{})},
            {"email", body.value("email", std::string{})},
            {"age",   body.value("age",   std::string{})}
        };

        Schema sch{
            {"name",  required("name")},
            {"age",   num_range(1, 150, "Age")},
            {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Invalid email")}
        };

        auto r = validate_map(data, sch);
        if (r.is_err())
        {
            J::Json e = J::o();
            for (const auto &kv : r.error())
                e[kv.first] = kv.second;

            res.status(Vix::http::status::bad_request)
               .json(J::o("errors", e));
            return;
        }

        User u;
        if (!parse_user(body, u))
        {
            res.status(Vix::http::status::bad_request)
               .json(J::o("error", "Invalid fields"));
            return;
        }

        // Generate ID (simple hash of email)
        u.id = std::to_string(std::hash<std::string>{}(u.email) & 0xFFFFFF);

        {
            std::lock_guard<std::mutex> lock(g_mtx);
            g_users[u.id] = u;
        }

        res.status(Vix::http::status::created)
           .json(J::o("status", "created", "user", to_json(u))); });

    // READ (GET /users/{id})
    app.get("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end())
        {
            res.status(Vix::http::status::not_found)
               .json(J::o("error", "User not found"));
            return;
        }
        res.json(J::o("user", to_json(it->second))); });

    // UPDATE (PUT /users/{id})
    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        J::Json body;
        try { body = J::loads(req.body()); }
        catch (...) {
            res.status(Vix::http::status::bad_request)
               .json(J::o("error", "Invalid JSON"));
            return;
        }

        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end())
        {
            res.status(Vix::http::status::not_found)
               .json(J::o("error", "User not found"));
            return;
        }

        if (body.contains("name"))  it->second.name  = body["name"].get<std::string>();
        if (body.contains("email")) it->second.email = body["email"].get<std::string>();
        if (body.contains("age"))
        {
            if (body["age"].is_string())
                it->second.age = std::stoi(body["age"].get<std::string>());
            else if (body["age"].is_number_integer())
                it->second.age = static_cast<int>(body["age"].get<long long>());
            else if (body["age"].is_number_unsigned())
                it->second.age = static_cast<int>(body["age"].get<unsigned long long>());
            else if (body["age"].is_number_float())
                it->second.age = static_cast<int>(body["age"].get<double>());
        }

        res.json(J::o("status", "updated", "user", to_json(it->second))); });

    // DELETE (DELETE /users/{id})
    app.del("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        auto n = g_users.erase(id);
        if (!n)
        {
            res.status(Vix::http::status::not_found)
               .json(J::o("error", "User not found"));
            return;
        }
        res.json(J::o("status", "deleted", "user_id", id)); });

    // ------------------------------------------------------------------------
    app.run(8080);
    return 0;
}
