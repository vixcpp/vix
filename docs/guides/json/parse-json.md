# Parse JSON

The parsing helpers read JSON from strings and files.

They are intentionally split into two families: strict functions that throw on failure, and safe functions that return `std::optional<Json>`. This makes error handling visible at the call site instead of hiding it behind one ambiguous API.

Use strict parsing when invalid JSON means the program cannot continue. Use safe parsing when the input comes from users, HTTP requests, external services, optional files, generated files, or any source that may be malformed.

```cpp id="ilfa9j"
#include <vix/json.hpp>

using namespace vix::json;

int main()
{
  Json config = loads(R"({"app":"Vix.cpp","debug":true})");

  return 0;
}
```

## Header

For normal application code, include:

```cpp id="mgkpp9"
#include <vix/json.hpp>
```

For direct usage of the parsing API only, include:

```cpp id="ztue74"
#include <vix/json/loads.hpp>
```

The parsing helpers live in:

```cpp id="dxw1xk"
namespace vix::json
```

## Public API

| API                   | Purpose                                                                |
| --------------------- | ---------------------------------------------------------------------- |
| `loads(text)`         | Parse JSON from a string and throw on invalid JSON.                    |
| `try_loads(text)`     | Parse JSON from a string and return `std::nullopt` on failure.         |
| `load_file(path)`     | Read and parse a JSON file, then throw on I/O or parse failure.        |
| `try_load_file(path)` | Read and parse a JSON file, then return `std::nullopt` on any failure. |

The return type for successful parsing is `Json`.

```cpp id="fbhiwu"
using Json = nlohmann::json;
```

## Parse a JSON string with `loads()`

Use `loads()` when the input is trusted or when invalid JSON should be treated as a hard error.

```cpp id="i4jzsy"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = loads(R"({
    "name": "Vix.cpp",
    "type": "runtime",
    "ok": true
  })");

  vix::print(dumps_pretty(data));

  return 0;
}
```

Output shape:

```json id="nh6wsn"
{
  "name": "Vix.cpp",
  "type": "runtime",
  "ok": true
}
```

`loads()` is strict. If the input is not valid JSON, the parser throws.

```cpp id="g3o2gy"
Json data = loads("not-json");
```

This is useful in tests, internal fixtures, required metadata, and code paths where invalid JSON indicates a programming error.

## Parse a JSON string safely with `try_loads()`

Use `try_loads()` when the input may be invalid.

```cpp id="cid9wc"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  const std::string input = R"({"name":"Ada","active":true})";

  auto data = try_loads(input);

  if (!data)
  {
    vix::print("Invalid JSON");
    return 1;
  }

  const std::string name = get_or<std::string>(*data, "name", "unknown");

  vix::print("name", name);

  return 0;
}
```

Output:

```txt id="n0qzlo"
name Ada
```

`try_loads()` never throws. It returns `std::nullopt` when parsing fails.

```cpp id="ksykd2"
auto data = try_loads("not-json");

if (!data)
{
  vix::print("Invalid JSON");
}
```

This style is better for user input and request bodies because failure stays local and explicit.

## Choose between `loads()` and `try_loads()`

The difference is not about performance first. The difference is about the error model.

| Situation                       | Recommended API            |
| ------------------------------- | -------------------------- |
| Required internal JSON literal  | `loads()`                  |
| Test fixture that must be valid | `loads()`                  |
| Required configuration content  | `loads()` or `load_file()` |
| HTTP request body               | `try_loads()`              |
| User input                      | `try_loads()`              |
| External service response       | `try_loads()`              |
| Optional file content           | `try_load_file()`          |

A backend route should usually avoid throwing for normal bad input. Invalid JSON in a request is not an internal failure. It is a client error that should be reported cleanly.

```cpp id="irf9vo"
auto body = try_loads(req.body());

if (!body)
{
  res.status(http::BAD_REQUEST).json({
    "error", "Invalid JSON"
  });
  return;
}
```

## Parse a JSON file with `load_file()`

Use `load_file()` when the file is required.

```cpp id="ekv7hv"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json config = load_file("config/app.json");

  const std::string app = get_or<std::string>(config, "app", "unknown");

  vix::print("app", app);

  return 0;
}
```

`load_file()` reads the entire file into memory, then parses it.

It throws when the file cannot be opened, when the file is empty, or when the content is not valid JSON.

This is appropriate for required configuration files because the application should not continue with an unknown configuration state.

## Parse a JSON file safely with `try_load_file()`

Use `try_load_file()` when the file is optional or may be invalid.

```cpp id="qzf8f6"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  auto config = try_load_file("config/local.json");

  if (!config)
  {
    vix::print("No local config loaded");
    return 0;
  }

  vix::print(dumps_pretty(*config));

  return 0;
}
```

`try_load_file()` catches any failure and returns `std::nullopt`.

This is useful for optional local overrides, cache files, generated metadata, or files that may be edited manually.

## Parse and validate fields

Parsing only proves that the input is valid JSON. It does not prove that the expected fields exist or have the correct type.

After parsing, use the conversion helpers to read the fields you need.

```cpp id="r9dpf5"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  auto data = try_loads(R"({
    "id": 42,
    "name": "Ada",
    "active": true
  })");

  if (!data)
  {
    vix::print("Invalid JSON");
    return 1;
  }

  const int id = get_or<int>(*data, "id", 0);
  const std::string name = get_or<std::string>(*data, "name", "unknown");
  const bool active = get_or<bool>(*data, "active", false);

  vix::print("id", id);
  vix::print("name", name);
  vix::print("active", active);

  return 0;
}
```

Output shape:

```txt id="svlphy"
id 42
name Ada
active true
```

For required fields, use `ensure<T>()`.

```cpp id="pm1gam"
const std::string name = ensure<std::string>(*data, "name");
```

This throws if the key is missing or if the value cannot be converted to the requested type.

For external input, it is often better to combine safe parsing with explicit validation.

```cpp id="qtwdpp"
auto name = get_opt<std::string>(*data, "name");

if (!name || name->empty())
{
  vix::print("Missing required field: name");
  return 1;
}
```

## Parse nested JSON

Nested structures can be accessed with normal `Json` operations, but `jget()` is often cleaner when the path is dynamic or deeply nested.

```cpp id="d5d3pc"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = loads(R"({
    "user": {
      "profile": {
        "name": "Ada",
        "country": "UG"
      },
      "roles": ["admin", "editor"]
    }
  })");

  const Json* name = jget(data, "user.profile.name");
  const Json* first_role = jget(data, "user.roles[0]");

  if (name)
  {
    vix::print("name", name->get<std::string>());
  }

  if (first_role)
  {
    vix::print("role", first_role->get<std::string>());
  }

  return 0;
}
```

Output shape:

```txt id="ob75da"
name Ada
role admin
```

`jget()` returns `nullptr` when the path is invalid or missing, so it works well after parsing external JSON.

## Parse JSON in an HTTP route

A Vix HTTP route receives the request body as a string. The JSON module can parse that body with `try_loads()`.

```cpp id="oo17zj"
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.post("/login", [](Request &req, Response &res) {
    using namespace vix::json;

    auto body = try_loads(req.body());

    if (!body)
    {
      res.status(http::BAD_REQUEST).json({
        "error", "Invalid JSON"
      });
      return;
    }

    const std::string email = get_or<std::string>(*body, "email", "");
    const std::string password = get_or<std::string>(*body, "password", "");

    if (email.empty() || password.empty())
    {
      res.status(http::UNPROCESSABLE_ENTITY).json({
        "error", "Missing credentials"
      });
      return;
    }

    res.json({
      "ok", true,
      "email", email
    });
  });

  app.run();

  return 0;
}
```

Test it:

```bash id="5coiyh"
curl -i \
  -X POST http://127.0.0.1:8080/login \
  -H "Content-Type: application/json" \
  -d '{"email":"ada@example.com","password":"secret"}'
```

Expected response shape:

```json id="e9uucz"
{
  "ok": true,
  "email": "ada@example.com"
}
```

The route uses `try_loads()` because invalid JSON is a normal client-side error, not a reason to crash the application.

## Avoid parsing the same body repeatedly

Parsing creates a JSON document. If a handler or middleware needs the parsed value several times, parse once and reuse the result.

```cpp id="shbbpq"
auto body = try_loads(req.body());

if (!body)
{
  res.status(http::BAD_REQUEST).json({
    "error", "Invalid JSON"
  });
  return;
}

const std::string name = get_or<std::string>(*body, "name", "");
const std::string email = get_or<std::string>(*body, "email", "");
```

Avoid this pattern:

```cpp id="bj8pnk"
auto name_body = try_loads(req.body());
auto email_body = try_loads(req.body());
```

The body should be parsed once, then fields should be read from the parsed `Json`.

## Large inputs

The file helpers read the entire document into memory. This is appropriate for configuration files, metadata files, fixtures, local cache files, and normal API payloads.

Do not use `load_file()` or `try_load_file()` for unbounded user uploads or very large streaming inputs. For those cases, use a streaming design at the application level.

This is a deliberate boundary: the Vix JSON helpers are for normal application JSON documents, not for arbitrary large data ingestion.

## Complete example

This example parses a JSON string safely, validates required fields, adds metadata, and prints the final document.

```cpp id="cvnrzs"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  const std::string input = R"({
    "name": "Ada",
    "email": "ada@example.com"
  })";

  auto data = try_loads(input);

  if (!data)
  {
    vix::print("Invalid JSON");
    return 1;
  }

  auto name = get_opt<std::string>(*data, "name");
  auto email = get_opt<std::string>(*data, "email");

  if (!name || name->empty())
  {
    vix::print("Missing field: name");
    return 1;
  }

  if (!email || email->empty())
  {
    vix::print("Missing field: email");
    return 1;
  }

  jset(*data, "meta.validated", true);
  jset(*data, "meta.source", "quick-start");

  vix::print(dumps_pretty(*data));

  return 0;
}
```

Output shape:

```json id="hczudd"
{
  "name": "Ada",
  "email": "ada@example.com",
  "meta": {
    "validated": true,
    "source": "quick-start"
  }
}
```

## Next steps

Continue with [Write JSON](/guides/json/write-json) to learn how to serialize JSON values and write JSON documents to files safely.
