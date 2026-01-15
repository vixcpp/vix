# Vix – Request & Response Examples

This document shows **complete, real-world examples** demonstrating how **simple, expressive, and powerful Vix** is.

No low-level types.
No `vhttp::`.
No `ResponseWrapper`.
Just **Request** and **Response**.

---

## 1. Hello World (JSON)

```cpp
app.get("/", [](Request req, Response res) {
    return json::o("message", "Hello from Vix");
});
```

---

## 2. Route Parameters

```cpp
app.get("/users/{id}", [](Request req, Response res) {
    auto id = req.param("id");
    return json::o("user_id", id);
});
```

---

## 3. Query Parameters

```cpp
app.get("/search", [](Request req, Response res) {
    auto q = req.query_value("q", "none");
    auto page = req.query_value("page", "1");

    return json::o(
        "query", q,
        "page", page
    );
});
```

---

## 4. Automatic Status + Payload (FastAPI style)

```cpp
app.get("/missing", [](Request req, Response res) {
    return std::pair{
        404,
        json::o("error", "Not found")
    };
});
```

---

## 5. Redirect

```cpp
app.get("/go", [](Request req, Response res) {
    res.redirect("https://vixcpp.com");
});
```

---

## 6. Automatic Status Message

```cpp
app.get("/forbidden", [](Request req, Response res) {
    res.status(403).send();
});
```

---

## 7. POST JSON Body

```cpp
app.post("/echo", [](Request req, Response res) {
    return json::o(
        "received", req.json()
    );
});
```

---

## 8. Typed JSON Parsing

```cpp
struct UserInput {
    std::string name;
    int age;
};

app.post("/users", [](Request req, Response res) {
    UserInput input = req.json_as<UserInput>();

    return std::pair{
        201,
        json::o(
            "name", input.name,
            "age", input.age
        )
    };
});
```

---

## 9. Headers

```cpp
app.get("/headers", [](Request req, Response res) {
    res.header("X-App", "Vix")
       .type("text/plain")
       .send("Hello headers");
});
```

---

## 10. Request-Scoped State

```cpp
app.get("/state", [](Request req, Response res) {
    req.set_state<int>(42);

    return json::o(
        "value", req.state<int>()
    );
});
```

---

## 11. Void Handler

```cpp
app.get("/manual", [](Request req, Response res) {
    res.status(200)
       .json(json::o("ok", true));
});
```

---

## 12. Params Map Access

```cpp
app.get("/items/{id}", [](Request req, Response res) {
    const auto& params = req.params();
    return json::o("id", params.at("id"));
});
```

---

## 13. 204 No Content

```cpp
app.delete("/items/{id}", [](Request req, Response res) {
    res.status(204).send();
});
```

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
    App app;

    // Basic JSON response (auto send)
    app.get("/", [](Request req, Response res) {
        res.send("message", "Hello from Vix");
    });
    // Path params + return {status, payload}
    app.get("/users/{id}", [](Request req, Response res) {
        auto id = req.param("id");
        return std::pair{200, vix::json::o("id", id)};
    });
    // Plain text return (const char*)
    app.get("/txt", [](const Request&, Response&) {
        return "Hello world";
    });
    // Redirect
    app.get("/go", [](Request req, Response res) {
        res.redirect("https://vixcpp.com");
    });
    // Status only → auto message (like Express sendStatus)
    app.get("/missing", [](Request req, Response res) {
        res.status(404).send();
    });
    // JSON echo (body → json)
    app.post("/echo", [](Request req, Response res) {
        return vix::json::o("received", req.json());
    });

    app.run(8080);
}
```

---

## Philosophy

- Zero magic
- Zero runtime overhead
- Compile-time safety
- Expressive like FastAPI / Express
- Pure modern C++

Vix lets you write **business logic**, not plumbing.
