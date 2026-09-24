# Quick start

::: warning Hardware requirements Vix.cpp runs everywhere, but
performance depends on your hardware.

For real-world production workloads, use a modern multi-core CPU, fast
I/O, and enough memory to avoid contention. GPU acceleration is optional
and only required for specialized compute workloads.
:::

## 1. Minimal server

Create a file `main.cpp`:

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.json({"message", "Hello, Vix!"});
  });

  app.get("/health", [](Request&, Response& res) {
    res.json({"ok", true, "service", "vix"});
  });

  app.run(8080);
}
```

This is a complete working server.

No configuration. No boilerplate. Just routes.

------------------------------------------------------------------------

## 2. Run

If you have Vix CLI installed:

``` bash
vix run main.cpp
```

Or compile normally with your C++ toolchain.

------------------------------------------------------------------------

## 3. Test

``` bash
curl http://127.0.0.1:8080/
curl http://127.0.0.1:8080/health
```

Expected:

``` json
{"message":"Hello, Vix!"}
```

``` json
{"ok":true,"service":"vix"}
```

------------------------------------------------------------------------

## 4. Minimal route examples

### Plain text

``` cpp
app.get("/txt", [](const Request&, Response&) {
  return "Hello world";
});
```

### Path parameter

``` cpp
app.get("/users/{id}", [](Request& req, Response& res) {
  const std::string id = req.param("id", "0");
  res.json({"id", id});
});
```

Test:

``` bash
curl http://127.0.0.1:8080/users/42
```

------------------------------------------------------------------------

### Query parameter

``` cpp
app.get("/search", [](Request& req, Response& res) {
  const std::string q = req.query_value("q", "");
  res.json({"q", q});
});
```

Test:

``` bash
curl "http://127.0.0.1:8080/search?q=vix"
```

------------------------------------------------------------------------

### Status code

``` cpp
app.get("/notfound", [](Request&, Response& res) {
  res.status(404).json({"error", "Not found"});
});
```

------------------------------------------------------------------------

## Philosophy

-   Routes are explicit.
-   No hidden magic.
-   You control status codes.
-   You control payload format.
-   The network layer stays predictable.

