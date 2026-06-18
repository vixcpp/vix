# Simple Token

`Simple.hpp` provides a lightweight JSON-like value model for Vix APIs that need structured values without depending directly on JSON text parsing or serialization.

Most application code should use `Json`.

Use `Simple` when an API specifically expects `token`, `kvs`, or `array_t`, or when a module needs to pass small structured values internally without constructing a full JSON document first.

```cpp id="s5b9ye"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  kvs user = obj({
    "name", "Ada",
    "age", 42,
    "skills", array({"C++", "Systems"})
  });

  Json json = to_json(user);

  vix::print(dumps_pretty(json));

  return 0;
}
```

Output shape:

```json id="nrk0o7"
{
  "age": 42,
  "name": "Ada",
  "skills": ["C++", "Systems"]
}
```

## Header

For normal application code, include:

```cpp id="zhidtg"
#include <vix/json.hpp>
```

For direct usage of the Simple model only, include:

```cpp id="ozmgv9"
#include <vix/json/Simple.hpp>
```

If you also want to convert Simple values to normal `Json`, include:

```cpp id="cnxmfe"
#include <vix/json/convert.hpp>
```

The Simple types live in:

```cpp id="v4gycb"
namespace vix::json
```

## What Simple represents

The Simple model can represent the same basic value categories as JSON:

| Value category | Simple representation         |
| -------------- | ----------------------------- |
| null           | `token{}` or `token(nullptr)` |
| boolean        | `token(true)`                 |
| integer        | stored as `std::int64_t`      |
| floating point | stored as `double`            |
| string         | stored as `std::string`       |
| array          | `array_t`                     |
| object         | `kvs`                         |

The central type is `token`.

A `token` is a tagged value. It can hold a primitive value directly, or hold an array/object through an internal shared pointer so recursive structures are possible.

## Public types

| Type      | Purpose                                                                   |
| --------- | ------------------------------------------------------------------------- |
| `token`   | A JSON-like value: null, bool, integer, double, string, array, or object. |
| `array_t` | A vector-like list of `token` values.                                     |
| `kvs`     | A flat key/value object representation.                                   |

There are also helper builders:

| Helper         | Purpose                                                |
| -------------- | ------------------------------------------------------ |
| `array({...})` | Build a Simple array.                                  |
| `obj({...})`   | Build a Simple object from flattened key/value tokens. |
| `to_json(...)` | Convert a Simple value to normal `Json`.               |

The names `obj()` and `array()` in `Simple.hpp` are separate from the normal JSON builders `o()` and `a()`. For normal JSON documents, prefer `o()` and `a()`.

## Create a token

A `token` can be constructed from the supported primitive types.

```cpp id="jdipmx"
#include <vix/json/Simple.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  token empty;
  token nothing = nullptr;
  token active = true;
  token count = 42;
  token price = 19.99;
  token name = "Ada";

  vix::print("tokens created");

  return 0;
}
```

Integer values are stored as `std::int64_t`. Floating point values are stored as `double`.

## Check token types

Use the type-checking methods before reading a value.

```cpp id="tmk86c"
#include <vix/json/Simple.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  token value = "Ada";

  if (value.is_string())
  {
    vix::print("string token");
  }

  return 0;
}
```

Common checks include:

| Method        | Meaning                      |
| ------------- | ---------------------------- |
| `is_null()`   | The token is null.           |
| `is_bool()`   | The token stores a boolean.  |
| `is_i64()`    | The token stores an integer. |
| `is_f64()`    | The token stores a double.   |
| `is_string()` | The token stores a string.   |
| `is_array()`  | The token stores an array.   |
| `is_object()` | The token stores an object.  |

## Read token values

Raw getters return pointers. If the token has another type, they return `nullptr`.

```cpp id="ul67ba"
#include <vix/json/Simple.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  token value = "Ada";

  if (const std::string* name = value.as_string())
  {
    vix::print("name", *name);
  }

  return 0;
}
```

Available raw getters include:

| Getter            | Returns                    |
| ----------------- | -------------------------- |
| `as_bool()`       | `const bool*`              |
| `as_i64()`        | `const std::int64_t*`      |
| `as_f64()`        | `const double*`            |
| `as_string()`     | `const std::string*`       |
| `as_array_ptr()`  | `std::shared_ptr<array_t>` |
| `as_object_ptr()` | `std::shared_ptr<kvs>`     |

The pointer-based API keeps wrong-type access explicit.

## Read with defaults

For primitive values, tokens also provide defaulted getters.

```cpp id="fedcdb"
#include <vix/json/Simple.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  token name = "Ada";
  token missing;

  std::string user_name = name.as_string_or("unknown");
  std::string fallback = missing.as_string_or("unknown");

  vix::print("user", user_name);
  vix::print("fallback", fallback);

  return 0;
}
```

Output shape:

```txt id="wzah4s"
user Ada
fallback unknown
```

Common defaulted getters:

| Method                  | Purpose                   |
| ----------------------- | ------------------------- |
| `as_bool_or(default)`   | Read bool or fallback.    |
| `as_i64_or(default)`    | Read integer or fallback. |
| `as_f64_or(default)`    | Read double or fallback.  |
| `as_string_or(default)` | Read string or fallback.  |

## Create a Simple array

Use `array({...})` to build an `array_t`.

```cpp id="ml1e3q"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  array_t skills = array({
    "C++",
    "Networking",
    "Systems"
  });

  Json json = to_json(skills);

  vix::print(dumps_pretty(json));

  return 0;
}
```

Output shape:

```json id="oryzk6"
["C++", "Networking", "Systems"]
```

`array_t` behaves like a small vector-like container around `std::vector<token>`.

```cpp id="jtjcvm"
array_t values;

values.push_int(42);
values.push_bool(true);
values.push_string("Ada");
```

Useful methods include:

| Method               | Purpose                                          |
| -------------------- | ------------------------------------------------ |
| `push_back(token)`   | Append a token.                                  |
| `push_null()`        | Append null.                                     |
| `push_bool(value)`   | Append bool.                                     |
| `push_int(value)`    | Append int.                                      |
| `push_i64(value)`    | Append int64.                                    |
| `push_f64(value)`    | Append double.                                   |
| `push_string(value)` | Append string.                                   |
| `ensure(index)`      | Grow the array if needed and return the element. |
| `erase_at(index)`    | Remove an element by index.                      |

## Create a Simple object

Use `obj({...})` to build a `kvs` object.

The initializer list is flattened:

```txt id="fd5opy"
key0, value0, key1, value1, ...
```

Example:

```cpp id="zfr8ln"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  kvs user = obj({
    "name", "Ada",
    "age", 42,
    "active", true
  });

  Json json = to_json(user);

  vix::print(dumps_pretty(json));

  return 0;
}
```

Output shape:

```json id="lb92x2"
{
  "active": true,
  "age": 42,
  "name": "Ada"
}
```

A `kvs` object stores pairs in a flat vector of tokens. The key should be a string token, followed by its value token.

## Work with object fields

Use `set()` to insert or replace a field.

```cpp id="yh944s"
kvs user;

user.set_string("name", "Ada");
user.set_i64("age", 42);
user.set_bool("active", true);
```

Use `get_*()` methods to read optional typed values.

```cpp id="dza6o1"
auto name = user.get_string("name");
auto age = user.get_i64("age");
```

Use `get_*_or()` methods when a default is appropriate.

```cpp id="f0kzp4"
std::string name = user.get_string_or("name", "unknown");
std::int64_t age = user.get_i64_or("age", 0);
bool active = user.get_bool_or("active", false);
```

Common object methods:

| Method                         | Purpose                                         |
| ------------------------------ | ----------------------------------------------- |
| `contains(key)`                | Check whether a key exists.                     |
| `get_ptr(key)`                 | Return pointer to the value token or `nullptr`. |
| `set(key, token)`              | Insert or replace a field.                      |
| `erase(key)`                   | Remove a field.                                 |
| `keys()`                       | Return the string keys.                         |
| `merge_from(other, overwrite)` | Merge another object.                           |
| `for_each_pair(fn)`            | Iterate over valid key/value pairs.             |

## Ensure nested arrays and objects

A `token` can be converted into an array or object in place.

```cpp id="yvr5ve"
token root;

kvs& object = root.ensure_object();
object.set_string("name", "Ada");

array_t& roles = object.ensure_array("roles");
roles.push_string("admin");
roles.push_string("editor");
```

This is useful when a payload is assembled progressively.

Complete example:

```cpp id="batkr8"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  token root;

  kvs& user = root.ensure_object();
  user.set_string("name", "Ada");
  user.set_i64("age", 42);

  array_t& roles = user.ensure_array("roles");
  roles.push_string("admin");
  roles.push_string("editor");

  Json json = to_json(root);

  vix::print(dumps_pretty(json));

  return 0;
}
```

Output shape:

```json id="vpe5sg"
{
  "age": 42,
  "name": "Ada",
  "roles": ["admin", "editor"]
}
```

## Convert Simple to Json

The bridge from Simple to normal JSON is `to_json(...)`.

```cpp id="p52xd7"
Json json = to_json(root);
```

Supported overloads include:

```cpp id="sgqyxg"
Json to_json(const token& value);
Json to_json(const kvs& object);
Json to_json(const array_t& array);
```

Conversion preserves the JSON-compatible meaning of the Simple value:

| Simple value | Converted JSON value |
| ------------ | -------------------- |
| null token   | `null`               |
| bool token   | JSON boolean         |
| int64 token  | JSON integer         |
| double token | JSON number          |
| string token | JSON string          |
| array token  | JSON array           |
| object token | JSON object          |

After conversion, you can use the normal JSON helpers:

```cpp id="dsvq9j"
Json json = to_json(user);

vix::print(dumps_pretty(json));
dump_file("user.json", json);
```

## Simple vs Json

Use `Json` for normal application data.

```cpp id="vgdjn4"
Json user = o(
  "name", "Ada",
  "age", 42
);
```

Use Simple when you need a lightweight structured value model, usually for internal APIs or response helpers that already accept `token`, `kvs`, or `array_t`.

```cpp id="iu20zp"
kvs user = obj({
  "name", "Ada",
  "age", 42
});
```

The difference is not that one is “better” than the other. They solve different problems.

`Json` is the general-purpose JSON document type. It can parse text, serialize text, integrate with `nlohmann::json`, and represent normal JSON documents.

`Simple` is a small JSON-like value system. It is useful when structured values need to be passed around inside Vix without requiring JSON parsing or direct construction of a `nlohmann::json` document at every layer.

## Use Simple in HTTP responses

Some Vix response helpers can convert Simple values into JSON responses. That allows small payloads to be expressed without constructing `Json` manually.

```cpp id="ye1d39"
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/simple", [](Request &, Response &res) {
    using namespace vix::json;

    kvs payload = obj({
      "ok", true,
      "message", "Hello from Simple"
    });

    res.json(to_json(payload));
  });

  app.run();

  return 0;
}
```

For normal route code, this is usually simpler:

```cpp id="dxsbn7"
res.json({
  "ok", true,
  "message", "Hello from Vix.cpp"
});
```

Use Simple in HTTP code only when another API already gives you a `token`, `kvs`, or `array_t`.

## Common mistakes

### Using Simple as the default JSON API

For normal JSON work, prefer:

```cpp id="n4dn02"
Json user = o(
  "name", "Ada",
  "age", 42
);
```

Do not start with Simple unless you need its lightweight model.

### Forgetting that `obj({...})` is flattened

This is valid:

```cpp id="wnfh4i"
kvs user = obj({
  "name", "Ada",
  "age", 42
});
```

The list is interpreted as:

```txt id="q710gt"
"name" -> "Ada"
"age"  -> 42
```

If the list has an odd number of values, the last value has no pair. Keep Simple object initializers balanced.

### Expecting `kvs` to be a map

`kvs` stores a flat vector of tokens, not a `std::map`.

This is intentional. It keeps the structure simple and preserves insertion-like behavior, but it also means object methods are responsible for searching and updating keys.

Use the provided methods instead of manipulating the flat vector directly unless you are writing low-level code.

### Ignoring wrong key types

Only string tokens are valid keys during normal object iteration and conversion.

```cpp id="kt4h0g"
kvs bad = obj({
  42, "value"
});
```

Avoid non-string keys. JSON object keys should be strings.

## Complete example

This example builds a Simple object progressively, reads fields with defaults, converts it to normal `Json`, writes it to a file, and prints it.

```cpp id="w28b5c"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  token root;

  kvs& user = root.ensure_object();
  user.set_string("name", "Ada");
  user.set_i64("age", 42);
  user.set_bool("active", true);

  array_t& skills = user.ensure_array("skills");
  skills.push_string("C++");
  skills.push_string("Networking");
  skills.push_string("Systems");

  const std::string name = user.get_string_or("name", "unknown");
  const std::int64_t age = user.get_i64_or("age", 0);

  user.set_string("display", name + " #" + std::to_string(age));

  Json json = to_json(root);

  dump_file("simple-user.json", json);

  vix::print(dumps_pretty(json));

  return 0;
}
```

Output shape:

```json id="pwk2dz"
{
  "active": true,
  "age": 42,
  "display": "Ada #42",
  "name": "Ada",
  "skills": ["C++", "Networking", "Systems"]
}
```

## Next steps

Continue with [JSON with HTTP](/guides/json/http) to see how the JSON module fits into Vix HTTP request and response handling.
