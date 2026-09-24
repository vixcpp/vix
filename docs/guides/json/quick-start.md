# JSON Quick Start

This page introduces the fastest way to use the Vix JSON module in an application.

The goal is not to cover every helper in detail. The goal is to show the normal workflow: include the module, create JSON values, parse input, read fields safely, write JSON, and use the same helpers inside a Vix HTTP route.

For most application code, include:

```cpp
#include <vix/json.hpp>
```

Then use the JSON helpers from:

```cpp
namespace vix::json
```

## Create a JSON value

The main JSON type is `Json`.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json app = {
    {"name", "Vix.cpp"},
    {"type", "runtime"},
    {"stable", true}
  };

  vix::print(dumps_pretty(app));

  return 0;
}
```

Output shape:

```json
{
  "name": "Vix.cpp",
  "type": "runtime",
  "stable": true
}
```

`Json` is an alias of `nlohmann::json`, so normal `nlohmann::json` syntax works.

Vix adds helpers for the patterns that appear often in application code.

## Build objects and arrays

Use `o()` to build an object and `a()` to build an array.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada",
    "active", true,
    "skills", a("C++", "Networking", "Systems")
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
  "active": true,
  "skills": ["C++", "Networking", "Systems"]
}
```

The `o()` helper expects alternating key/value arguments:

```cpp
auto user = o("id", 42, "name", "Ada");
```

This is easier to read in route responses, configuration generation, and tests.

## Parse JSON from a string

Use `loads()` when invalid JSON should throw.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = loads(R"({
    "name": "Vix.cpp",
    "version": "2.6",
    "ok": true
  })");

  vix::print(dumps_pretty(data));

  return 0;
}
```

Use `try_loads()` when the input can be invalid.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  auto data = try_loads(R"({"name":"Vix.cpp","ok":true})");

  if (!data)
  {
    vix::print("Invalid JSON");
    return 1;
  }

  vix::print(dumps_pretty(*data));

  return 0;
}
```

In real applications, `try_loads()` is usually the better choice for request bodies, user input, files from external systems, or anything that may be malformed.

## Read fields safely

A JSON field can be missing or have the wrong type. Instead of checking every field manually, use `get_or()` or `get_opt()`.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada"
  );

  int id = get_or<int>(user, "id", 0);
  std::string name = get_or<std::string>(user, "name", "unknown");
  std::string email = get_or<std::string>(user, "email", "none");

  vix::print("id", id);
  vix::print("name", name);
  vix::print("email", email);

  return 0;
}
```

Output shape:

```txt
id 42
name Ada
email none
```

Use `get_opt<T>()` when the field is optional and you want to know whether it exists.

```cpp
if (auto email = get_opt<std::string>(user, "email"))
{
  vix::print("email", *email);
}
else
{
  vix::print("email missing");
}
```

Use `ensure<T>()` when the field is required.

```cpp
std::string required_name = ensure<std::string>(user, "name");
```

If the key is missing or the type is wrong, `ensure<T>()` throws.

## Work with nested data

For nested JSON values, use `jget()` to read by path.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = loads(R"({
    "user": {
      "name": "Ada",
      "roles": ["admin", "editor"]
    }
  })");

  const Json* name = jget(data, "user.name");
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

```txt
name Ada
role admin
```

Use `jset()` to write nested values.

```cpp
jset(data, "user.profile.country", "UG");
jset(data, "user.roles[1]", "developer");
```

After this, `data` contains the new nested fields.

## Serialize JSON

Use `dumps_pretty()` when the output is meant to be read by a person.

```cpp
std::string readable = dumps_pretty(data);
```

Use `dumps_compact()` when the output is meant for a network response, compact storage, or single-line logs.

```cpp
std::string compact = dumps_compact(data);
```

Example:

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json event = o(
    "type", "user.created",
    "user_id", 42,
    "ok", true
  );

  vix::print(dumps_compact(event));

  return 0;
}
```

Output:

```json
{ "type": "user.created", "user_id": 42, "ok": true }
```

## Read and write JSON files

Use `dump_file()` to write a JSON document.

```cpp
#include <vix/json.hpp>

using namespace vix::json;

int main()
{
  Json config = o(
    "app", "Vix.cpp",
    "debug", true,
    "port", 8080
  );

  dump_file("config/app.json", config);

  return 0;
}
```

Use `load_file()` when the file must exist and must be valid.

```cpp
Json config = load_file("config/app.json");
```

Use `try_load_file()` when the file is optional or may be invalid.

```cpp
auto config = try_load_file("config/app.json");

if (!config)
{
  return 1;
}
```

## Use JSON in a Vix HTTP route

The JSON helpers can be used directly inside Vix HTTP handlers.

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

Test the route:

```bash
curl -i \
  -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada"}'
```

Expected response shape:

```json
{
  "ok": true,
  "name": "Ada"
}
```

Invalid JSON produces an error response:

```bash
curl -i \
  -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d 'not-json'
```

Expected response shape:

```json
{
  "error": "Invalid JSON"
}
```

## A complete quick-start example

This example combines the main pieces: building JSON, reading fields, writing nested values, and printing the final document.

```cpp
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json user = o(
    "id", 42,
    "name", "Ada",
    "roles", a("admin", "editor")
  );

  const std::string name = get_or<std::string>(user, "name", "unknown");

  jset(user, "profile.country", "UG");
  jset(user, "profile.display_name", name);

  dump_file("user.json", user);

  Json reloaded = load_file("user.json");

  vix::print(dumps_pretty(reloaded));

  return 0;
}
```

Output shape:

```json
{
  "id": 42,
  "name": "Ada",
  "roles": ["admin", "editor"],
  "profile": {
    "country": "UG",
    "display_name": "Ada"
  }
}
```

This is the normal JSON workflow in Vix: build a value, read what you need safely, update it when necessary, serialize it, and use the same approach in HTTP handlers or standalone tools.

## Next steps

Continue with [Build JSON](/guides/json/build-json) to learn the object and array builders in more detail.
