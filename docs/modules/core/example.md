
# Vix.cpp Examples Guide (Beginner Friendly)

Welcome to Vix.cpp.

This guide is designed for developers coming from:

- Python (FastAPI, Flask)
- Node.js (Express)
- Simple HTTP servers in other languages

The goal is to show you how to think in Vix.

Vix is:
- Explicit
- Predictable
- No magic
- No hidden runtime behavior
- C++ performance with modern syntax

------------------------------------------------------------
1. Minimal Server (Hello World)
------------------------------------------------------------

In Node.js (Express):
```js
    app.get("/", (req, res) => {
        res.json({ message: "Hello" });
    });
```
In Vix:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
    App app;

    app.get("/", [](Request&, Response& res) {
        res.json({"message", "Hello Vix"});
    });

    app.run(8080);
}
```

Run it:
```bash
    vix run main.cpp
```

Open:
```bash
    http://localhost:8080
```
------------------------------------------------------------
2. Returning JSON Automatically
------------------------------------------------------------

In Vix you can either:

- Use res.json(...)
- Or return JSON directly

Example:

```cpp
app.get("/auto", [](Request&, Response&) {
    return vix::json::o(
        "framework", "vix",
        "language", "c++",
        "fast", true
    );
});
```

If you return a value, Vix auto-sends it.

------------------------------------------------------------
3. Path Parameters (Like Express :id)
------------------------------------------------------------

Node.js:

    app.get("/users/:id", ...)

Vix:

```cpp
app.get("/users/{id}", [](Request& req, Response& res) {
    std::string id = req.param("id", "0");

    res.json({
        "id", id,
        "name", "User#" + id
    });
});
```

Test:
```bash
    http://localhost:8080/users/42
```

------------------------------------------------------------
4. Query Parameters
------------------------------------------------------------

Example:

    /search?q=vix&page=2

```cpp
app.get("/search", [](Request& req, Response& res) {

    std::string q = req.query_value("q", "");
    std::string pageStr = req.query_value("page", "1");

    res.json({
        "query", q,
        "page", pageStr
    });
});
```

------------------------------------------------------------
5. Status Codes
------------------------------------------------------------

```cpp
app.get("/created", [](Request&, Response& res) {
    res.status(201).json({
        "status", 201,
        "message", "Created"
    });
});
```

------------------------------------------------------------
6. Reading Headers
------------------------------------------------------------

```cpp
app.get("/headers", [](Request& req, Response& res) {

    std::string ua = req.header("User-Agent");

    res.json({
        "user_agent", ua
    });
});
```

------------------------------------------------------------
7. Reading JSON Body
------------------------------------------------------------

Even though GET is used here for demo purposes,
normally POST is used.

```cpp
app.post("/echo", [](Request& req, Response& res) {

    auto j = req.json();

    if (!j.is_object()) {
        res.status(400).json({"error", "Invalid JSON"});
        return;
    }

    res.json({
        "received", j
    });
});
```

------------------------------------------------------------
8. Returning Text
------------------------------------------------------------

```cpp
app.get("/text", [](Request&, Response&) {
    return "Plain text response";
});
```

------------------------------------------------------------
9. Organizing Routes
------------------------------------------------------------

Instead of writing everything in main(), do this:

```cpp
void register_routes(App& app)
{
    app.get("/", [](Request&, Response& res){
        res.json({"ok", true});
    });

    app.get("/health", [](Request&, Response& res){
        res.json({"status", "healthy"});
    });
}

int main()
{
    App app;
    register_routes(app);
    app.run(8080);
}
```

Keep main() clean.

------------------------------------------------------------
10. Disabling Auto Docs
------------------------------------------------------------

By default Vix can generate OpenAPI docs.

To disable:
```bash
    vix run api.cpp --no-docs
```

------------------------------------------------------------
Mental Model for Python / Node Developers
------------------------------------------------------------

In Python or Node:
- The runtime hides many details.
- Middleware stacks are dynamic.
- Garbage collection is automatic.

In Vix:
- Everything is explicit.
- No garbage collector.
- No hidden async magic.
- You control memory and execution.

This gives:

- Lower latency
- Higher predictability
- Better performance under load

------------------------------------------------------------
Where to Go Next
------------------------------------------------------------

After this guide:

- Explore Session and ThreadPool for advanced execution
- Enable OpenAPI and test /docs
- Add your own real API routes
- Connect to a database
- Deploy to production

Vix is simple at the surface.
Powerful underneath.

Keep building.

