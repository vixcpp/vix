# Build JSON

The JSON builders provide a compact way to create JSON objects and arrays in Vix.cpp code.

They are useful when the JSON value is created directly in C++, for example in API responses, configuration generation, tests, fixtures, metadata files, and small internal payloads.

The builders do not replace `nlohmann::json`. They are a thin convenience layer on top of it. The main JSON type remains `vix::json::Json`, which is an alias of `nlohmann::json`.

```cpp
#include <vix/json.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada",
    "active", true,
    "skills", a("C++", "Networking", "Systems")
  );

  return 0;
}
```

## Header

For normal application code, include:

```cpp
#include <vix/json.hpp>
```

For direct usage of the builder API only, include:

```cpp
#include <vix/json/build.hpp>
```

The builder helpers live in:

```cpp
namespace vix::json
```

## Public API

| API                                | Purpose                                                            |
| ---------------------------------- | ------------------------------------------------------------------ |
| `o(k1, v1, k2, v2, ...)`           | Build an ordered JSON object from alternating key/value arguments. |
| `a(v1, v2, ...)`                   | Build a JSON array from values.                                    |
| `kv({ {"key", Json(value)}, ...})` | Build a JSON object from a list of key/`Json` pairs.               |
| `obj()`                            | Create an empty JSON object.                                       |
| `arr()`                            | Create an empty JSON array.                                        |

The most common helpers are `o()` and `a()`.

## Build an object with `o()`

Use `o()` when the object keys are known in the source code.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada",
    "role", "admin"
  );

  vix::print(dumps_pretty(user));

  return 0;
}
```

Output shape:

```json
{
  "id": 42,
  "name": "Ada",
  "role": "admin"
}
```

The function expects alternating key/value arguments:

```cpp
auto value = o(
  "key1", value1,
  "key2", value2,
  "key3", value3
);
```

The number of arguments must be even. This is checked at compile time.

Valid:

```cpp
auto user = o("name", "Ada", "age", 42);
```

Invalid:

```cpp
auto user = o("name", "Ada", "age");
```

The invalid version has a key without a value.

## Why `o()` returns `OrderedJson`

The `o()` helper returns `OrderedJson`.

```cpp
using OrderedJson = nlohmann::ordered_json;
```

This means object iteration and output follow insertion order. That is useful when the generated JSON is read by humans, compared in tests, or used in documentation examples.

```cpp
auto payload = o(
  "ok", true,
  "id", 42,
  "name", "Ada"
);
```

The output keeps the same logical order:

```json
{
  "ok": true,
  "id": 42,
  "name": "Ada"
}
```

This is not required by JSON itself, but it makes examples, configuration files, and generated payloads easier to inspect.

## Build an array with `a()`

Use `a()` to build a JSON array from C++ values.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json languages = a("C++", "Python", "JavaScript");

  vix::print(dumps_pretty(languages));

  return 0;
}
```

Output shape:

```json
["C++", "Python", "JavaScript"]
```

The array can contain mixed JSON-compatible values:

```cpp
Json values = a(
  42,
  true,
  "Vix.cpp",
  nullptr
);
```

Output shape:

```json
[42, true, "Vix.cpp", null]
```

## Build nested JSON

The builders can be nested.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada",
    "profile", o(
      "country", "UG",
      "city", "Kampala"
    ),
    "roles", a("admin", "editor")
  );

  vix::print(dumps_pretty(user));

  return 0;
}
```

Output shape:

```json
{
  "id": 42,
  "name": "Ada",
  "profile": {
    "country": "UG",
    "city": "Kampala"
  },
  "roles": ["admin", "editor"]
}
```

This style is especially useful for API responses because the structure of the JSON remains visible in the C++ code.

## Build an empty object

Use `obj()` when you want to start with an empty object and fill it progressively.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = obj();

  user["id"] = 42;
  user["name"] = "Ada";
  user["active"] = true;

  vix::print(dumps_pretty(user));

  return 0;
}
```

Output shape:

```json
{
  "active": true,
  "id": 42,
  "name": "Ada"
}
```

Use `obj()` when fields are conditional, computed in different parts of the function, or easier to assign one by one.

## Build an empty array

Use `arr()` when you want to create an array and append values later.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json items = arr();

  items.push_back("first");
  items.push_back("second");
  items.push_back("third");

  vix::print(dumps_pretty(items));

  return 0;
}
```

Output shape:

```json
["first", "second", "third"]
```

Use `arr()` when the number of elements is not known at the point where the array is created.

## Build an object from pairs with `kv()`

Use `kv()` when the key/value pairs are already available as a list.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json config = kv({
    {"app", Json("Vix.cpp")},
    {"debug", Json(true)},
    {"port", Json(8080)}
  });

  vix::print(dumps_pretty(config));

  return 0;
}
```

Output shape:

```json
{
  "app": "Vix.cpp",
  "debug": true,
  "port": 8080
}
```

The values passed to `kv()` must be `Json` values.

This is valid:

```cpp
Json config = kv({
  {"name", Json("Vix.cpp")},
  {"port", Json(8080)}
});
```

This is not the intended form:

```cpp
Json config = kv({
  {"name", "Vix.cpp"},
  {"port", 8080}
});
```

When the keys and values are known directly in the call site, prefer `o()`:

```cpp
Json config = o(
  "name", "Vix.cpp",
  "port", 8080
);
```

Use `kv()` when a pair-based representation already exists or when the object is being assembled from data that is naturally stored as key/value entries.

## Use builders in HTTP responses

In many Vix HTTP examples, JSON responses are written directly through `res.json(...)`.

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/health", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "service", "api"
    });
  });

  app.run();

  return 0;
}
```

For more complex payloads, the JSON builders can make the response easier to prepare before sending it.

```cpp
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/users/{id}", [](Request &req, Response &res) {
    using namespace vix::json;

    const std::string id = req.param("id");

    Json payload = o(
      "ok", true,
      "user", o(
        "id", id,
        "name", "Ada"
      ),
      "links", o(
        "self", "/users/" + id
      )
    );

    res.json(payload);
  });

  app.run();

  return 0;
}
```

The route still follows the normal Vix HTTP style: the handler receives `Request &req` and `Response &res`, then writes into `res`.

The JSON builder is only used to prepare the payload.

## Use builders for configuration files

The builders are also useful when generating configuration or metadata files.

```cpp
#include <vix/json.hpp>

using namespace vix::json;

int main()
{
  Json config = o(
    "server", o(
      "host", "127.0.0.1",
      "port", 8080
    ),
    "log", o(
      "level", "info",
      "format", "kv"
    )
  );

  dump_file("config/app.json", config);

  return 0;
}
```

Generated file shape:

```json
{
  "server": {
    "host": "127.0.0.1",
    "port": 8080
  },
  "log": {
    "level": "info",
    "format": "kv"
  }
}
```

For generated files, deterministic object order is useful because diffs remain easier to read.

## Use builders in tests

Tests often need small JSON fixtures.

```cpp
#include <vix/json.hpp>

using namespace vix::json;

Json make_user_fixture()
{
  return o(
    "id", 42,
    "name", "Ada",
    "active", true,
    "roles", a("admin", "editor")
  );
}
```

This avoids long setup code and keeps the shape of the fixture visible.

## When to use each builder

Use `o()` when the JSON object is known at the call site.

```cpp
auto user = o("id", 42, "name", "Ada");
```

Use `a()` when the JSON array is known at the call site.

```cpp
auto roles = a("admin", "editor");
```

Use `obj()` when the object is filled progressively.

```cpp
Json user = obj();
user["id"] = 42;
user["name"] = "Ada";
```

Use `arr()` when the array is filled progressively.

```cpp
Json items = arr();
items.push_back("one");
items.push_back("two");
```

Use `kv()` when you already have key/`Json` pairs.

```cpp
Json data = kv({
  {"name", Json("Ada")},
  {"active", Json(true)}
});
```

## Common mistakes

### Odd number of arguments in `o()`

This is invalid:

```cpp
auto user = o("id", 42, "name");
```

Every key must have a value:

```cpp
auto user = o("id", 42, "name", "Ada");
```

### Passing non-key values as keys

Keys passed to `o()` must be convertible to `std::string_view`.

Good:

```cpp
auto user = o("name", "Ada");
```

Avoid using values that are not string-like as keys:

```cpp
auto user = o(42, "Ada");
```

### Using `kv()` when `o()` is clearer

This works:

```cpp
Json user = kv({
  {"id", Json(42)},
  {"name", Json("Ada")}
});
```

But this is usually clearer:

```cpp
Json user = o(
  "id", 42,
  "name", "Ada"
);
```

### Building very large JSON documents in one expression

For small and medium payloads, nested builders are readable.

For large documents, prefer a progressive structure:

```cpp
Json root = obj();
root["users"] = arr();

for (const auto& user : users)
{
  root["users"].push_back(o(
    "id", user.id,
    "name", user.name
  ));
}
```

This keeps the code easier to debug and avoids deeply nested expressions.

## Complete example

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json project = o(
    "name", "Vix.cpp",
    "kind", "runtime",
    "modules", a(
      o("name", "core", "enabled", true),
      o("name", "json", "enabled", true),
      o("name", "websocket", "enabled", true)
    ),
    "metadata", o(
      "language", "C++",
      "format", "json"
    )
  );

  vix::print(dumps_pretty(project));

  return 0;
}
```

Output shape:

```json
{
  "name": "Vix.cpp",
  "kind": "runtime",
  "modules": [
    {
      "name": "core",
      "enabled": true
    },
    {
      "name": "json",
      "enabled": true
    },
    {
      "name": "websocket",
      "enabled": true
    }
  ],
  "metadata": {
    "language": "C++",
    "format": "json"
  }
}
```

## Next steps

Continue with [Parse JSON](/guides/json/parse-json) to learn how to parse JSON from strings and files.
