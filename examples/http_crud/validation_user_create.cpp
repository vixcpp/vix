// ============================================================================
// users_crud_validated.cpp — CRUD + validation (new Vix.cpp API)
// ============================================================================

#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <vix/utils/Validation.hpp>
#include <nlohmann/json.hpp>

#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

using namespace vix;
namespace J = vix::json;
using njson = nlohmann::json;
using namespace vix::utils;

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

// --------------------------- JSON helpers (Simple API) ----------------------
static J::kvs to_json(const User &u)
{
    return J::obj({"id", u.id,
                   "name", u.name,
                   "email", u.email,
                   "age", static_cast<long long>(u.age)});
}

static std::string j_to_string(const njson &j, const char *k)
{
    if (!j.contains(k))
        return {};
    const auto &v = j.at(k);
    if (v.is_string())
        return v.get<std::string>();
    if (v.is_number_integer())
        return std::to_string(v.get<long long>());
    if (v.is_number_unsigned())
        return std::to_string(v.get<unsigned long long>());
    if (v.is_number_float())
        return std::to_string(v.get<double>());
    if (v.is_boolean())
        return v.get<bool>() ? "true" : "false";
    return v.dump(); // objet/array -> JSON string
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

// --------------------------- App -------------------------------------------
int main()
{
    App app;

    // CREATE
    app.post("/users", [](auto &req, auto &res)
             {
        njson body;
        try {
            body = njson::parse(req.body());
        } catch (...) {
            res.status(http::status::bad_request).json({ "error", "Invalid JSON" });
            return;
        }

        std::unordered_map<std::string, std::string> data{
            {"name",  j_to_string(body, "name")},
            {"email", j_to_string(body, "email")},
            {"age",   j_to_string(body, "age")}
        };

        Schema sch{
            {"name",  required("name")},
            {"age",   num_range(1, 150, "Age")},
            {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Invalid email")}
        };

        auto r = validate_map(data, sch);
        if (r.is_err()) {
           // Construire {"errors": {field: message, ...}} sans nlohmann
            std::vector<J::token> flat;
            flat.reserve(r.error().size() * 2);
            for (const auto& kv : r.error()) {
                flat.emplace_back(kv.first);   // clé
                flat.emplace_back(kv.second);  // valeur
            }

            // J::obj(std::move(flat)) crée un objet JSON à partir de la liste plate
            res.status(400).json({
                "errors", J::obj(std::move(flat))
            });
            return;
        }

        User u;
        if (!parse_user(body, u)) {
            res.status(400).json({ "error", "Invalid fields" });
            return;
        }

        // Simple ID from email hash
        u.id = std::to_string(std::hash<std::string>{}(u.email) & 0xFFFFFF);

        {
            std::lock_guard<std::mutex> lock(g_mtx);
            g_users[u.id] = u;
        }

        res.status(http::status::created).json({
            "status", "created",
            "user",   to_json(u)
        }); });

    // READ
    app.get("/users/{id}", [](auto &, auto &res, auto &params)
            {
        const std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end()) {
            res.status(http::status::not_found).json({ "error", "Not found" });
            return;
        }
        res.json({ "user", to_json(it->second) }); });

    // UPDATE
    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        const std::string id = params["id"];

        njson body;
        try {
            body = njson::parse(req.body());
        } catch (...) {
            res.status(http::status::bad_request).json({ "error", "Invalid JSON" });
            return;
        }

        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end()) {
            res.status(http::status::not_found).json({ "error", "Not found" });
            return;
        }

        if (body.contains("name"))  it->second.name  = body["name"].get<std::string>();
        if (body.contains("email")) it->second.email = body["email"].get<std::string>();
        if (body.contains("age")) {
            if      (body["age"].is_string())               it->second.age = std::stoi(body["age"].get<std::string>());
            else if (body["age"].is_number_integer())       it->second.age = static_cast<int>(body["age"].get<long long>());
            else if (body["age"].is_number_unsigned())      it->second.age = static_cast<int>(body["age"].get<unsigned long long>());
            else if (body["age"].is_number_float())         it->second.age = static_cast<int>(body["age"].get<double>());
        }

        res.json({ "status", "updated", "user", to_json(it->second) }); });

    // DELETE
    app.del("/users/{id}", [](auto &, auto &res, auto &params)
            {
        const std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        const auto n = g_users.erase(id);
        if (!n) {
            res.status(http::status::not_found).json({ "error", "Not found" });
            return;
        }
        res.json({ "status", "deleted", "user_id", id }); });

    app.run(8080);
    return 0;
}
