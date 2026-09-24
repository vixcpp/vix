# Safe Access

JSON documents often come from places the program does not fully control: HTTP requests, configuration files, external services, local cache files, generated metadata, and user input.

Parsing proves that the input is valid JSON. It does not prove that the fields your code expects are present, nor that they have the right type.

The safe access helpers reduce repetitive checks around JSON values. They make it easier to read optional fields, provide defaults, or fail clearly when a required value is missing.

```cpp id="la4cod"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = loads(R"({
    "id": 42,
    "name": "Ada",
    "active": true
  })");

  const int id = get_or<int>(user, "id", 0);
  const std::string name = get_or<std::string>(user, "name", "unknown");
  const std::string email = get_or<std::string>(user, "email", "none");

  vix::print("id", id);
  vix::print("name", name);
  vix::print("email", email);

  return 0;
}
```

Output shape:

```txt id="0dbpq1"
id 42
name Ada
email none
```

## Header

For normal application code, include:

```cpp id="jym8ce"
#include <vix/json.hpp>
```

For direct usage of the safe access API only, include:

```cpp id="9ri7oz"
#include <vix/json/convert.hpp>
```

The safe access helpers live in:

```cpp id="m6u8jy"
namespace vix::json
```

## Public API

| API                                | Purpose                                                                 |
| ---------------------------------- | ----------------------------------------------------------------------- |
| `ptr(json, key)`                   | Return a pointer to an object member, or `nullptr`.                     |
| `ptr(json, index)`                 | Return a pointer to an array element, or `nullptr`.                     |
| `get_opt<T>(json)`                 | Convert a JSON value to `T`, returning `std::nullopt` on failure.       |
| `get_opt<T>(json, key)`            | Read and convert an object field, returning `std::nullopt` on failure.  |
| `get_opt<T>(json, index)`          | Read and convert an array element, returning `std::nullopt` on failure. |
| `get_or<T>(json, fallback)`        | Convert a JSON value to `T`, or return a fallback.                      |
| `get_or<T>(json, key, fallback)`   | Read and convert an object field, or return a fallback.                 |
| `get_or<T>(json, index, fallback)` | Read and convert an array element, or return a fallback.                |
| `ensure<T>(json)`                  | Strictly convert a JSON value to `T`.                                   |
| `ensure<T>(json, key)`             | Strictly read and convert a required object field.                      |
| `to_json(...)`                     | Convert Vix `Simple` values to normal `Json`.                           |

The API gives you three practical levels of strictness:

| Level            | Function       | Behavior                                           |
| ---------------- | -------------- | -------------------------------------------------- |
| Pointer access   | `ptr()`        | Missing values become `nullptr`.                   |
| Optional access  | `get_opt<T>()` | Missing or invalid values become `std::nullopt`.   |
| Defaulted access | `get_or<T>()`  | Missing or invalid values become a fallback value. |
| Strict access    | `ensure<T>()`  | Missing or invalid values throw.                   |

## Pointer access with `ptr()`

Use `ptr()` when you want to check whether a field or array element exists without copying it.

```cpp id="flj3q9"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada"
  );

  const Json* name = ptr(user, "name");
  const Json* email = ptr(user, "email");

  if (name)
  {
    vix::print("name", name->get<std::string>());
  }

  if (!email)
  {
    vix::print("email missing");
  }

  return 0;
}
```

Output shape:

```txt id="no5id0"
name Ada
email missing
```

`ptr(json, key)` returns `nullptr` if the value is not an object or if the key does not exist.

For arrays, use the index overload:

```cpp id="napz3w"
Json roles = a("admin", "editor");

const Json* first = ptr(roles, 0);
const Json* third = ptr(roles, 2);
```

The second pointer is `nullptr` because index `2` is out of bounds.

## Optional access with `get_opt()`

Use `get_opt<T>()` when a value is optional and the code needs to know whether the conversion succeeded.

```cpp id="ihpvyc"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada"
  );

  if (auto name = get_opt<std::string>(user, "name"))
  {
    vix::print("name", *name);
  }

  if (auto email = get_opt<std::string>(user, "email"))
  {
    vix::print("email", *email);
  }
  else
  {
    vix::print("email missing");
  }

  return 0;
}
```

Output shape:

```txt id="whugf4"
name Ada
email missing
```

`get_opt<T>()` returns `std::nullopt` when the field is missing, when the JSON value is `null`, when the value is discarded, or when `nlohmann::json::get<T>()` fails.

This is useful for optional request fields, optional config values, and external payloads where the program should continue after a missing field.

## Defaulted access with `get_or()`

Use `get_or<T>()` when a missing or invalid value should become a default.

```cpp id="n8e1rp"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json config = o(
    "host", "127.0.0.1",
    "debug", true
  );

  const std::string host = get_or<std::string>(config, "host", "localhost");
  const int port = get_or<int>(config, "port", 8080);
  const bool debug = get_or<bool>(config, "debug", false);

  vix::print("host", host);
  vix::print("port", port);
  vix::print("debug", debug);

  return 0;
}
```

Output shape:

```txt id="mty4th"
host 127.0.0.1
port 8080
debug true
```

This keeps default behavior close to the field being read.

It is clearer than writing separate `contains()` checks for every optional value.

## Strict access with `ensure()`

Use `ensure<T>()` when a field is required.

```cpp id="xv0mgn"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada"
  );

  const int id = ensure<int>(user, "id");
  const std::string name = ensure<std::string>(user, "name");

  vix::print("id", id);
  vix::print("name", name);

  return 0;
}
```

Output shape:

```txt id="3t85nm"
id 42
name Ada
```

`ensure<T>(json, key)` throws if the value is not an object, if the key is missing, or if the value cannot be converted to `T`.

Use this for trusted data and required internal structures.

For external request bodies, prefer `get_opt<T>()` or `get_or<T>()` first, then return a clear error response.

## Access array elements safely

The same helpers work with array indexes.

```cpp id="fgt7xb"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json roles = a("admin", "editor");

  const std::string first = get_or<std::string>(roles, 0, "none");
  const std::string third = get_or<std::string>(roles, 2, "none");

  vix::print("first", first);
  vix::print("third", third);

  return 0;
}
```

Output shape:

```txt id="ht0tni"
first admin
third none
```

This avoids manual bounds checks when the array comes from a JSON payload.

## Convert a direct JSON value

`get_opt<T>()`, `get_or<T>()`, and `ensure<T>()` can also convert a JSON value directly.

```cpp id="u9cuz2"
Json value = 42;

int number = get_or<int>(value, 0);
```

For optional conversion:

```cpp id="0uvvd8"
Json value = "42";

auto number = get_opt<int>(value);

if (!number)
{
  vix::print("not an integer");
}
```

For strict conversion:

```cpp id="4ialko"
Json value = "Ada";

std::string name = ensure<std::string>(value);
```

## Validate external payloads

Safe access is most useful when validation is part of the application logic.

```cpp id="rp4yx2"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  auto body = try_loads(R"({
    "name": "Ada",
    "email": "ada@example.com"
  })");

  if (!body)
  {
    vix::print("Invalid JSON");
    return 1;
  }

  auto name = get_opt<std::string>(*body, "name");
  auto email = get_opt<std::string>(*body, "email");

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

  vix::print("valid user", *name, *email);

  return 0;
}
```

This style separates the steps clearly:

1. parse the JSON,
2. read fields safely,
3. validate required application rules,
4. continue only when the payload is valid.

## Use safe access in HTTP routes

In HTTP handlers, invalid input should normally produce a response, not an uncaught exception.

```cpp id="wa4bsj"
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

    auto name = get_opt<std::string>(*body, "name");
    auto email = get_opt<std::string>(*body, "email");

    if (!name || name->empty())
    {
      res.status(http::UNPROCESSABLE_ENTITY).json({
        "error", "Missing required field",
        "field", "name"
      });
      return;
    }

    if (!email || email->empty())
    {
      res.status(http::UNPROCESSABLE_ENTITY).json({
        "error", "Missing required field",
        "field", "email"
      });
      return;
    }

    res.status(http::CREATED).json({
      "ok", true,
      "name", *name,
      "email", *email
    });
  });

  app.run();

  return 0;
}
```

The route uses `try_loads()` because the request body may be invalid, and `get_opt()` because required fields must be validated before they are used.

## `get_or()` vs `get_opt()`

`get_or()` is better when the field is optional and a default value is part of the application behavior.

```cpp id="bjdg03"
const int page = get_or<int>(query, "page", 1);
const int limit = get_or<int>(query, "limit", 20);
```

`get_opt()` is better when the application must distinguish between “missing” and “present”.

```cpp id="r66wmc"
auto email = get_opt<std::string>(body, "email");

if (!email)
{
  // missing or invalid
}
```

In validation code, prefer `get_opt()` because a default can hide invalid input.

In configuration code, `get_or()` is often appropriate because default values are expected.

## `ensure()` vs safe functions

`ensure()` is useful when the value must exist and a failure indicates an internal error.

```cpp id="d9tzpl"
Json package = load_file("vix.json");

const std::string name = ensure<std::string>(package, "name");
const std::string version = ensure<std::string>(package, "version");
```

This is reasonable when the file is required and the application cannot continue without those fields.

For client input, avoid using `ensure()` as the first step unless you catch the exception and convert it into a response.

```cpp id="m5uxot"
try
{
  const std::string name = ensure<std::string>(*body, "name");
}
catch (const std::exception& e)
{
  res.status(http::UNPROCESSABLE_ENTITY).json({
    "error", e.what()
  });
  return;
}
```

In most routes, explicit `get_opt()` validation is clearer.

## Access nested values

For nested values, combine safe access with `jget()`.

```cpp id="aw93kw"
Json data = loads(R"({
  "user": {
    "profile": {
      "name": "Ada"
    }
  }
})");

const Json* profile_name = jget(data, "user.profile.name");

std::string name = get_or<std::string>(profile_name, "unknown");
```

This works because `get_or<T>()` also accepts a `const Json*`.

If the path is missing, `jget()` returns `nullptr`, and `get_or()` returns the fallback.

## Convert `Simple` values to `Json`

The conversion header also bridges the lightweight `Simple` value model to normal JSON.

```cpp id="fz6rp2"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  kvs user = obj({
    "name", "Ada",
    "age", 42,
    "roles", array({"admin", "editor"})
  });

  Json json = to_json(user);

  vix::print(dumps_pretty(json));

  return 0;
}
```

Output shape:

```json id="y8h8oo"
{
  "age": 42,
  "name": "Ada",
  "roles": ["admin", "editor"]
}
```

Most application code should use `Json` directly. The `Simple` model is useful when a Vix API uses `token`, `kvs`, or `array_t` internally.

## Common mistakes

### Treating valid JSON as valid application data

This is valid JSON:

```json id="hhof0x"
{
  "name": 42
}
```

But it may be invalid for an endpoint that expects `name` to be a string.

Parsing should be followed by field access and validation.

```cpp id="q4ha76"
auto name = get_opt<std::string>(body, "name");

if (!name)
{
  // wrong type or missing field
}
```

### Using defaults for required fields

This hides bad input:

```cpp id="u2zrr3"
std::string email = get_or<std::string>(body, "email", "");
```

For a required field, prefer:

```cpp id="rv67gz"
auto email = get_opt<std::string>(body, "email");

if (!email || email->empty())
{
  // return validation error
}
```

### Calling `.get<T>()` everywhere

Direct `.get<T>()` is fine for trusted data.

```cpp id="ang07z"
std::string name = data["name"].get<std::string>();
```

For external data, this spreads exception handling across the code. Prefer `get_opt()`, `get_or()`, or a dedicated validation step.

### Ignoring arrays

Array indexes can be missing too.

```cpp id="p07w4y"
std::string first = get_or<std::string>(roles, 0, "none");
```

This is safer than assuming the array has at least one element.

## Complete example

This example parses a JSON payload, validates fields, reads optional settings, updates metadata, and prints the final JSON document.

```cpp id="ti7t7w"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  auto payload = try_loads(R"({
    "name": "Ada",
    "email": "ada@example.com",
    "settings": {
      "newsletter": true
    }
  })");

  if (!payload)
  {
    vix::print("Invalid JSON");
    return 1;
  }

  auto name = get_opt<std::string>(*payload, "name");
  auto email = get_opt<std::string>(*payload, "email");

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

  const Json* newsletter_value = jget(*payload, "settings.newsletter");
  const bool newsletter = get_or<bool>(newsletter_value, false);

  jset(*payload, "meta.validated", true);
  jset(*payload, "meta.newsletter", newsletter);

  vix::print(dumps_pretty(*payload));

  return 0;
}
```

Output shape:

```json id="kyfl1o"
{
  "name": "Ada",
  "email": "ada@example.com",
  "settings": {
    "newsletter": true
  },
  "meta": {
    "validated": true,
    "newsletter": true
  }
}
```

## Next steps

Continue with [JPath](/guides/json/jpath) to learn how to read and write nested JSON values using path expressions.
