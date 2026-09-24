# JPath

JPath is a small path syntax for reading and writing nested JSON values.

It is useful when a JSON document contains nested objects or arrays and the code needs to access values without writing long chains of `operator[]` calls.

JPath is intentionally limited. It is not a full JSONPath implementation. It supports the operations Vix applications commonly need: object keys, array indexes, and quoted keys inside brackets.

```cpp id="y8m1rx"
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
  const Json* role = jget(data, "user.roles[0]");

  if (name)
  {
    vix::print("name", name->get<std::string>());
  }

  if (role)
  {
    vix::print("role", role->get<std::string>());
  }

  return 0;
}
```

Output shape:

```txt id="iief1d"
name Ada
role admin
```

## Header

For normal application code, include:

```cpp id="y26qxs"
#include <vix/json.hpp>
```

For direct usage of the JPath API only, include:

```cpp id="s9wjia"
#include <vix/json/jpath.hpp>
```

The JPath helpers live in:

```cpp id="ez96fa"
namespace vix::json
```

## Public API

| API                       | Purpose                                                           |
| ------------------------- | ----------------------------------------------------------------- |
| `jget(const Json&, path)` | Read a nested value and return `const Json*` or `nullptr`.        |
| `jget(Json&, path)`       | Get a mutable nested value and create missing intermediate nodes. |
| `jset(json, path, value)` | Assign a value at a path and return `true` or `false`.            |
| `tokenize_path(path)`     | Parse a path into structured tokens, throwing on syntax errors.   |

The most common functions are `jget()` and `jset()`.

## Supported syntax

JPath supports dot notation for object keys:

```txt id="a1hwqx"
user.name
settings.theme
profile.address.city
```

It supports array indexes:

```txt id="18li9v"
users[0].name
roles[1]
items[2].price
```

It also supports quoted keys inside brackets. This is useful when a key contains a dot, a space, or another character that would be ambiguous in dot notation.

```txt id="o9bimj"
["complex.key"].value
user["display name"]
metadata["x-vix-id"]
```

The parser is small and direct. It does not support wildcards, filters, recursive descent, expressions, or JSONPath-style queries.

## Read a nested value

Use the const overload of `jget()` to read from a JSON document.

```cpp id="r782r7"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json config = loads(R"({
    "server": {
      "host": "127.0.0.1",
      "port": 8080
    }
  })");

  const Json* host = jget(config, "server.host");
  const Json* port = jget(config, "server.port");

  if (host)
  {
    vix::print("host", host->get<std::string>());
  }

  if (port)
  {
    vix::print("port", port->get<int>());
  }

  return 0;
}
```

Output shape:

```txt id="xe0u1u"
host 127.0.0.1
port 8080
```

The const overload does not create anything. It returns `nullptr` when the path is invalid, when a key is missing, when an array index is out of bounds, or when the path expects an object or array but the current value has another type.

## Read array values

Array indexes use square brackets.

```cpp id="lqnlt0"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = loads(R"({
    "users": [
      {"name": "Ada"},
      {"name": "Bjarne"}
    ]
  })");

  const Json* first = jget(data, "users[0].name");
  const Json* second = jget(data, "users[1].name");
  const Json* missing = jget(data, "users[2].name");

  if (first)
  {
    vix::print("first", first->get<std::string>());
  }

  if (second)
  {
    vix::print("second", second->get<std::string>());
  }

  if (!missing)
  {
    vix::print("third user missing");
  }

  return 0;
}
```

Output shape:

```txt id="wwm2wv"
first Ada
second Bjarne
third user missing
```

Indexes are zero-based. Negative indexes are not supported.

## Use quoted keys

Dot notation treats `.` as a path separator. If the actual object key contains a dot, use quoted bracket notation.

```cpp id="p6cv3h"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = o(
    "metadata", o(
      "x.vix.id", "abc-123"
    )
  );

  const Json* value = jget(data, "metadata[\"x.vix.id\"]");

  if (value)
  {
    vix::print(value->get<std::string>());
  }

  return 0;
}
```

Output:

```txt id="ff5chc"
abc-123
```

Quoted keys support basic escaping for quotes and backslashes.

```txt id="ir875p"
["key with spaces"]
["key.with.dots"]
["key\"with\"quotes"]
["path\\name"]
```

Use quoted keys only when needed. For normal keys, dot notation is easier to read.

## Combine `jget()` with safe access

`jget()` returns a pointer. That works well with `get_or()` and `get_opt()`.

```cpp id="e6gnm2"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = loads(R"({
    "settings": {
      "pagination": {
        "page": 2,
        "limit": 50
      }
    }
  })");

  const Json* page_value = jget(data, "settings.pagination.page");
  const Json* limit_value = jget(data, "settings.pagination.limit");

  const int page = get_or<int>(page_value, 1);
  const int limit = get_or<int>(limit_value, 20);

  vix::print("page", page);
  vix::print("limit", limit);

  return 0;
}
```

Output shape:

```txt id="mfddbz"
page 2
limit 50
```

If the path is missing, `jget()` returns `nullptr`, and `get_or()` returns the fallback.

This is a clean pattern for nested configuration and external payloads.

## Write a nested value with `jset()`

Use `jset()` to assign a value at a path.

```cpp id="prtlxq"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = obj();

  jset(data, "user.name", "Ada");
  jset(data, "user.profile.country", "UG");
  jset(data, "user.roles[0]", "admin");
  jset(data, "user.roles[1]", "editor");

  vix::print(dumps_pretty(data));

  return 0;
}
```

Output shape:

```json id="ym7j2a"
{
  "user": {
    "name": "Ada",
    "profile": {
      "country": "UG"
    },
    "roles": ["admin", "editor"]
  }
}
```

`jset()` creates missing intermediate values. Missing keys become objects. Missing array positions grow the array and are filled with `null` when necessary.

## Array expansion

When writing to an array index that does not exist yet, JPath expands the array.

```cpp id="p0phcw"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = obj();

  jset(data, "items[2]", "third");

  vix::print(dumps_pretty(data));

  return 0;
}
```

Output shape:

```json id="rot97z"
{
  "items": [null, null, "third"]
}
```

This behavior is useful when building payloads progressively, but it should be used carefully. If a large index comes from user input, validate it before using it.

## Mutable `jget()`

The non-const overload of `jget()` returns a mutable pointer and creates missing nodes as it walks the path.

```cpp id="vnxqwb"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = obj();

  Json* value = jget(data, "server.port");

  if (value)
  {
    *value = 8080;
  }

  vix::print(dumps_pretty(data));

  return 0;
}
```

Output shape:

```json id="ogcl34"
{
  "server": {
    "port": 8080
  }
}
```

For most application code, `jset()` is clearer when the goal is assignment. Use mutable `jget()` when you need a reference to modify a nested value directly.

## Invalid paths

The read-only overload of `jget()` does not throw. It returns `nullptr` for invalid syntax or missing values.

```cpp id="tlho7c"
const Json* value = jget(data, "users[bad]");

if (!value)
{
  vix::print("missing or invalid path");
}
```

The mutable overload may throw when the path syntax is invalid because it needs to distinguish between a missing value it can create and a path expression that cannot be parsed.

`jset()` catches errors and returns `false`.

```cpp id="8kkbcc"
if (!jset(data, "users[bad]", "Ada"))
{
  vix::print("failed to assign path");
}
```

This makes `jset()` convenient in application code where a failed assignment should not throw through the whole operation.

## Tokenizing a path

`tokenize_path()` parses a JPath string into tokens.

Most application code does not need it directly. It is useful for diagnostics, tooling, tests, or APIs that need to inspect the path before using it.

```cpp id="ghe1xe"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  auto tokens = tokenize_path("users[0].name");

  vix::print("segments", tokens.size());

  return 0;
}
```

A token is either a key segment or an index segment.

```cpp id="sdt8ou"
struct Token
{
  enum Kind
  {
    Key,
    Index
  } kind;

  std::string key;
  std::size_t index;
};
```

`tokenize_path()` throws on invalid syntax.

## JPath in configuration code

JPath is useful when configuration values are nested but the application wants a concise lookup.

```cpp id="hf3lye"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json config = loads(R"({
    "server": {
      "host": "127.0.0.1",
      "port": 8080
    },
    "log": {
      "level": "info"
    }
  })");

  const std::string host = get_or<std::string>(
    jget(config, "server.host"),
    "localhost"
  );

  const int port = get_or<int>(
    jget(config, "server.port"),
    8080
  );

  const std::string level = get_or<std::string>(
    jget(config, "log.level"),
    "warn"
  );

  vix::print("host", host);
  vix::print("port", port);
  vix::print("level", level);

  return 0;
}
```

Output shape:

```txt id="q1575o"
host 127.0.0.1
port 8080
level info
```

This is easier to maintain than repeating nested object checks at every level.

## JPath in payload construction

JPath can also be used to build a nested payload from separate pieces of code.

```cpp id="mzrw49"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

void add_user(Json& out)
{
  jset(out, "user.id", 42);
  jset(out, "user.name", "Ada");
}

void add_metadata(Json& out)
{
  jset(out, "meta.source", "example");
  jset(out, "meta.validated", true);
}

int main()
{
  Json payload = obj();

  add_user(payload);
  add_metadata(payload);

  vix::print(dumps_pretty(payload));

  return 0;
}
```

Output shape:

```json id="cxx2yh"
{
  "user": {
    "id": 42,
    "name": "Ada"
  },
  "meta": {
    "source": "example",
    "validated": true
  }
}
```

This pattern is useful when different functions contribute to the same output document.

## JPath in HTTP routes

In HTTP handlers, JPath is useful when request bodies contain nested data.

```cpp id="zu8dgb"
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.post("/profiles", [](Request &req, Response &res) {
    using namespace vix::json;

    auto body = try_loads(req.body());

    if (!body)
    {
      res.status(http::BAD_REQUEST).json({
        "error", "Invalid JSON"
      });
      return;
    }

    const std::string name = get_or<std::string>(
      jget(*body, "user.profile.name"),
      ""
    );

    if (name.empty())
    {
      res.status(http::UNPROCESSABLE_ENTITY).json({
        "error", "Missing required field",
        "field", "user.profile.name"
      });
      return;
    }

    res.json({
      "ok", true,
      "name", name
    });
  });

  app.run();

  return 0;
}
```

Example request:

```bash id="gch3rv"
curl -i \
  -X POST http://127.0.0.1:8080/profiles \
  -H "Content-Type: application/json" \
  -d '{"user":{"profile":{"name":"Ada"}}}'
```

Expected response shape:

```json id="o0n53o"
{
  "ok": true,
  "name": "Ada"
}
```

The route is still a normal Vix route. JPath is only used to access nested JSON cleanly.

## Common mistakes

### Treating JPath as JSONPath

JPath is not a query language.

It does not support:

```txt id="kmtl38"
users[*].name
users[?(@.active)]
..name
```

Use it for direct navigation, not searching.

### Using unvalidated indexes from users

This can create large arrays:

```cpp id="j8f7ub"
jset(data, "items[1000000]", "value");
```

If the index comes from input, validate it before writing.

### Using dot notation for keys that contain dots

This path:

```txt id="mv2dnc"
metadata.x.vix.id
```

means:

```txt id="oaoy9d"
metadata -> x -> vix -> id
```

If the actual key is `x.vix.id`, use quoted brackets:

```txt id="q86owv"
metadata["x.vix.id"]
```

### Ignoring `nullptr` from read access

This is unsafe:

```cpp id="k8id95"
std::string name = jget(data, "user.name")->get<std::string>();
```

Always check the pointer or combine it with `get_or()`:

```cpp id="nrv26l"
std::string name = get_or<std::string>(
  jget(data, "user.name"),
  "unknown"
);
```

## Complete example

This example reads nested values, applies defaults, writes metadata, and prints the updated document.

```cpp id="gb7lvx"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json data = loads(R"({
    "user": {
      "profile": {
        "name": "Ada"
      },
      "roles": ["admin"]
    },
    "settings": {
      "theme": "dark"
    }
  })");

  const std::string name = get_or<std::string>(
    jget(data, "user.profile.name"),
    "unknown"
  );

  const std::string first_role = get_or<std::string>(
    jget(data, "user.roles[0]"),
    "none"
  );

  const std::string language = get_or<std::string>(
    jget(data, "settings.language"),
    "en"
  );

  jset(data, "meta.name", name);
  jset(data, "meta.first_role", first_role);
  jset(data, "meta.language", language);
  jset(data, "meta.validated", true);

  vix::print(dumps_pretty(data));

  return 0;
}
```

Output shape:

```json id="xnb70c"
{
  "user": {
    "profile": {
      "name": "Ada"
    },
    "roles": ["admin"]
  },
  "settings": {
    "theme": "dark"
  },
  "meta": {
    "name": "Ada",
    "first_role": "admin",
    "language": "en",
    "validated": true
  }
}
```

## Next steps

Continue with [Simple Token](/guides/json/simple-token) to learn about the lightweight JSON-like value model used by some Vix APIs.
