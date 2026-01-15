//
// examples/http/router_params.cpp
//
// Example demonstrating route parameters with Vix.cpp.
//
// Demonstrates:
//   - Path parameters: /hello/{name}
//   - Multiple parameters: /posts/{year}/{slug}
//   - Returning JSON with dynamic values
//

#include <vix.hpp>

using namespace vix;

int main()
{
    App app;

    // GET /hello/{name}
    //
    // Example:
    //   GET /hello/Alice  -> { "greeting": "Hello Alice 👋" }
    //
    app.get("/hello/{name}", [](Request &req, Response &res)
            {
        const auto name = req.param("name");
        res.json({
            "greeting",   "Hello " + name + " 👋",
            "powered_by", "Vix.cpp"
        }); });

    // GET /posts/{year}/{slug}
    //
    // Example:
    //   GET /posts/2025/hello-world
    //
    app.get("/posts", [](Request &req, Response &res)
            {
        const auto year = req.query_value("year");
        const auto slug = req.query_value("slug");

        res.json(json::kv({
            {"year",       year},
            {"slug",       slug},
            {"title",      "Post: " + slug},
            {"message",    "This is an example route with multiple params."},
            {"powered_by", "Vix.cpp"},
            {"params", req.params()},
            {"query", req.query()}
        })); });

    // Optional: a root route for discoverability
    app.get("/", [](Request &, Response &res)
            { res.json({"routes", "/hello/{name}, /posts/{year}/{slug}",
                        "hint", "Try GET /hello/Alice or /posts/2025/hello-world"}); });

    app.run(8080);
    return 0;
}
