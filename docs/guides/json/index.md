# JSON

The Vix JSON module provides a small set of utilities for working with JSON in Vix.cpp applications.

It is built on top of `nlohmann::json`, so it does not introduce a separate JSON format or a custom data model for normal application code. Instead, it gives Vix projects a more direct way to build JSON values, parse input, write files, access fields safely, and navigate nested payloads.

Most applications use JSON in several places: HTTP request bodies, API responses, configuration files, metadata, test fixtures, logs, and internal module payloads. The JSON module keeps those operations explicit while reducing repetitive boilerplate.

```cpp
#include <vix/json.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada",
    "skills", a("C++", "Networking", "Systems")
  );

  const std::string name = get_or<std::string>(user, "name", "unknown");

  dump_file("user.json", user);

  return 0;
}
```

## What the JSON module provides

The module is organized around the common JSON operations used in application code.

| Area          | Purpose                                                                          |
| ------------- | -------------------------------------------------------------------------------- |
| Builders      | Create JSON objects and arrays with less syntax noise.                           |
| Parsing       | Parse JSON from strings or files, either strictly or safely.                     |
| Serialization | Convert JSON values to strings and write JSON documents to disk.                 |
| Safe access   | Read fields without repeating `contains()`, type checks, and exception handling. |
| JPath         | Read or write nested values using a small path syntax.                           |
| Simple token  | Use a lightweight JSON-like value model for internal Vix APIs.                   |
| HTTP usage    | Use JSON helpers with Vix request and response objects.                          |

The main entry point is:

```cpp
#include <vix/json.hpp>
```

This header exposes the common JSON utilities in one place. It is the recommended include for application code.

For lower-level or compile-time-sensitive code, individual headers can be included directly:

```cpp
#include <vix/json/build.hpp>
#include <vix/json/loads.hpp>
#include <vix/json/dumps.hpp>
#include <vix/json/convert.hpp>
#include <vix/json/jpath.hpp>
#include <vix/json/Simple.hpp>
```

## JSON types

The module exposes two main aliases:

```cpp
using Json = nlohmann::json;
using OrderedJson = nlohmann::ordered_json;
```

`Json` is the default JSON type. It is fully compatible with `nlohmann::json`, so existing code that already uses `nlohmann::json` can work with Vix JSON helpers without conversion.

`OrderedJson` preserves object insertion order. Vix uses it in the object builder so generated JSON remains deterministic and easier to read in examples, tests, configuration files, and API responses.

```cpp
Json data = {
  {"name", "Vix.cpp"},
  {"type", "runtime"}
};

OrderedJson ordered = o(
  "name", "Vix.cpp",
  "type", "runtime"
);
```

## Building JSON values

The builder helpers are designed for code that creates JSON values directly.

```cpp
auto response = o(
  "ok", true,
  "service", "users",
  "items", a("one", "two", "three")
);
```

The most common helpers are:

| Helper           | Purpose                                              |
| ---------------- | ---------------------------------------------------- |
| `o(k1, v1, ...)` | Build an ordered JSON object from key/value pairs.   |
| `a(v1, v2, ...)` | Build a JSON array.                                  |
| `kv(...)`        | Build a JSON object from a list of key/`Json` pairs. |
| `obj()`          | Create an empty JSON object.                         |
| `arr()`          | Create an empty JSON array.                          |

Example:

```cpp
Json config = o(
  "app", "Vix.cpp",
  "debug", true,
  "features", a("http", "json", "websocket")
);
```

The `o()` helper expects an even number of arguments:

```cpp
auto valid = o("name", "Ada", "age", 42);
```

This is invalid because the last key has no value:

```cpp
auto invalid = o("name", "Ada", "age");
```

## Parsing JSON

The parsing API separates strict parsing from safe parsing.

Use `loads()` when invalid JSON is a programming error and should stop execution:

```cpp
Json data = loads(R"({"name":"Vix.cpp","ok":true})");
```

Use `try_loads()` when the input comes from outside the program:

```cpp
auto data = try_loads(input);

if (!data)
{
  // invalid JSON
  return;
}
```

For files, the same pattern exists:

```cpp
Json config = load_file("config.json");
```

or:

```cpp
auto config = try_load_file("config.json");

if (!config)
{
  // file missing, empty, unreadable, or invalid
  return;
}
```

This distinction matters in backend and CLI applications. A required configuration file should usually fail loudly. A request body, user input, or optional file should usually be handled without throwing through the request path.

## Writing JSON

The serialization helpers convert JSON values to readable or compact strings.

```cpp
std::string pretty = dumps(data);
std::string compact = dumps_compact(data);
```

Use pretty JSON when humans will read the output:

```cpp
vix::print(dumps_pretty(data));
```

Use compact JSON for network payloads, storage, or logs where a single-line representation is better:

```cpp
std::string body = dumps_compact(data);
```

To write a JSON document to disk:

```cpp
dump_file("config/app.json", data);
```

`dump_file()` writes through a temporary file before replacing the destination. This reduces the chance of leaving a partially written file when a process fails during the write.

## Safe access

JSON payloads often contain missing fields, optional fields, or values with the wrong type. The conversion helpers make this explicit.

```cpp
std::string name = get_or<std::string>(data, "name", "unknown");
int age = get_or<int>(data, "age", 0);
```

Use `get_opt<T>()` when the value is optional:

```cpp
if (auto email = get_opt<std::string>(data, "email"))
{
  vix::print("email", *email);
}
```

Use `ensure<T>()` when the value is required and a missing or invalid field should be treated as an error:

```cpp
std::string id = ensure<std::string>(data, "id");
```

The practical rule is simple: use safe access for external data, and strict access when the data is trusted or required.

## Navigating nested JSON with JPath

For nested structures, the JPath helpers avoid long chains of `operator[]` calls.

```cpp
Json data = loads(R"({
  "user": {
    "name": "Ada",
    "roles": ["admin", "editor"]
  }
})");

const Json* role = jget(data, "user.roles[0]");

if (role)
{
  vix::print(role->get<std::string>());
}
```

For writes, `jset()` creates missing intermediate objects and arrays when needed:

```cpp
jset(data, "user.profile.country", "UG");
jset(data, "user.roles[1]", "developer");
```

JPath is intentionally small. It supports object keys, array indexes, and quoted keys inside brackets. It is not meant to be a complete JSONPath implementation.

## Using JSON with HTTP

The JSON module is useful on its own, but it is also used naturally with Vix HTTP applications.

A route can parse the request body with `try_loads()`, validate the fields with `get_or()` or `ensure()`, and send JSON through `res.json(...)`.

```cpp
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.post("/users", [](Request &req, Response &res) {
    using namespace vix::json;

    auto body = try_loads(req.body());

    if (!body)
    {
      res.status(http::BAD_REQUEST).json({
        "error", "Invalid JSON"
      });
      return;
    }

    const std::string name = get_or<std::string>(*body, "name", "");

    if (name.empty())
    {
      res.status(http::UNPROCESSABLE_ENTITY).json({
        "error", "Missing required field",
        "field", "name"
      });
      return;
    }

    res.status(http::CREATED).json({
      "ok", true,
      "name", name
    });
  });

  app.run();

  return 0;
}
```

The HTTP integration is not the center of the JSON module, but it is one of the most common ways JSON appears in real Vix applications. The important part is that the same helpers used for standalone JSON work can also be used inside request handlers.

## Simple token model

Most application code should use `Json`.

The `Simple.hpp` header provides a smaller JSON-like value model with `token`, `array_t`, and `kvs`. It is useful when a module needs to pass structured values internally without parsing or serializing JSON text.

```cpp
using namespace vix::json;

kvs user = obj({
  "name", "Ada",
  "age", 42,
  "skills", array({"C++", "Systems"})
});

token root = user;
```

This model can represent null, booleans, integers, floating point values, strings, arrays, and objects. It can also be converted to normal `Json` when needed.

For most users, `Simple.hpp` is an advanced part of the module. Start with `Json`, then use `Simple` only when a Vix API specifically expects it or when you need a lightweight structured value type.

## Headers

| Header                   | Use                                            |
| ------------------------ | ---------------------------------------------- |
| `<vix/json.hpp>`         | Main JSON include for application code.        |
| `<vix/json/json.hpp>`    | JSON module entry point under the module path. |
| `<vix/json/build.hpp>`   | Object and array builders.                     |
| `<vix/json/loads.hpp>`   | String and file parsing.                       |
| `<vix/json/dumps.hpp>`   | String and file serialization.                 |
| `<vix/json/convert.hpp>` | Safe field access and conversion helpers.      |
| `<vix/json/jpath.hpp>`   | Nested JSON navigation and mutation.           |
| `<vix/json/Simple.hpp>`  | Lightweight JSON-like token model.             |

## Recommended reading order

Read the JSON guide in this order:

1. [Quick Start](/guides/json/quick-start)
2. [Build JSON](/guides/json/build-json)
3. [Parse JSON](/guides/json/parse-json)
4. [Write JSON](/guides/json/write-json)
5. [Safe Access](/guides/json/safe-access)
6. [JPath](/guides/json/jpath)
7. [Simple Token](/guides/json/simple-token)
8. [JSON with HTTP](/guides/json/http)

This page gives the module overview. The next pages explain each part with focused examples and API details.
