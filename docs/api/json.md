# JSON API

This page documents the JSON facilities available in Vix.

Vix provides two JSON layers:

1)  Simple JSON (`vix/json/Simple.hpp`)
2)  High-level JSON (`vix/json/json.hpp`)

They serve different purposes and can coexist.

------------------------------------------------------------------------

# 1) Simple JSON

Header:

``` cpp
#include <vix/json/Simple.hpp>
```

Namespace:

``` cpp
vix::json
```

Core types:

-   `token` → generic JSON value
-   `array_t` → JSON array
-   `kvs` → JSON object storage

------------------------------------------------------------------------

## token

Represents any JSON value.

Supported kinds:

-   null
-   bool
-   int64
-   double
-   string
-   array
-   object

Example:

``` cpp
#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  token t = 42;

  if (t.is_i64())
    std::cout << t.as_i64_or(0) << "\n";

  return 0;
}
```

------------------------------------------------------------------------

## array_t

Dynamic JSON array.

Key methods:

-   `push_int(...)`
-   `push_string(...)`
-   `push_bool(...)`
-   `size()`
-   `operator[]`

Example:

``` cpp
#include <vix/json/Simple.hpp>

using namespace vix::json;

int main()
{
  array_t arr;
  arr.push_int(1);
  arr.push_string("two");

  return 0;
}
```

------------------------------------------------------------------------

## kvs

Object-like key/value storage.

Key methods:

-   `set_string(key, value)`
-   `set_int(key, value)`
-   `set_bool(key, value)`
-   `get_string_or(key, fallback)`
-   `ensure_object(key)`
-   `ensure_array(key)`
-   `merge_from(other, overwrite)`
-   `erase(key)`
-   `keys()`

Example:

``` cpp
#include <vix/json/Simple.hpp>

using namespace vix::json;

int main()
{
  kvs obj;
  obj.set_string("name", "Vix");

  return 0;
}
```

------------------------------------------------------------------------

# 2) High-Level JSON

Header:

``` cpp
#include <vix/json/json.hpp>
```

Powered by nlohmann::json.

Namespace helpers:

``` cpp
vix::json
```

Core type:

``` cpp
Json
```

------------------------------------------------------------------------

## Object Builder

``` cpp
#include <vix/json/json.hpp>

using namespace vix::json;

int main()
{
  auto j = o(
    "name", "Vix",
    "version", 1
  );

  return 0;
}
```

------------------------------------------------------------------------

## Array Builder

``` cpp
auto arr = a(1, 2, 3);
```

------------------------------------------------------------------------

## kv initializer

``` cpp
auto j = kv({
  {"a", 1},
  {"b", true}
});
```

------------------------------------------------------------------------

## dumps

Serialize with indentation:

``` cpp
std::string s = dumps(j, 2);
```

------------------------------------------------------------------------

## loads

Parse string:

``` cpp
auto j = loads(R"({"a":1})");
```

------------------------------------------------------------------------

## File IO

``` cpp
dump_file("out.json", j, 2);
auto j2 = load_file("out.json");
```

------------------------------------------------------------------------

## jset / jget (Path Access)

Mutate nested structures using path strings.

``` cpp
Json j = obj();

jset(j, "user.profile.name", "Ada");

if (auto v = jget(j, "user.profile.name"))
{
  // value exists
}
```

------------------------------------------------------------------------

# Design Notes

Simple JSON:

-   Minimal overhead
-   Strong control
-   Useful for internal state and WebSocket payloads

High-level JSON:

-   Expressive builders
-   API responses
-   Parsing and file operations

The JSON API is explicit and deterministic.

