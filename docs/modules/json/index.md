# JSON

Vix provides two JSON layers:

1.  Simple JSON (`vix/json/Simple.hpp`) --- lightweight, minimal, no
    external dependency.
2.  High-level JSON (`vix/json/json.hpp`) --- powered by nlohmann::json
    with expressive builders.

Use Simple when you want full control and minimal overhead. Use
High-level when you want expressive object construction and easy
serialization.

------------------------------------------------------------------------

# 1) Simple JSON

Include:

``` cpp
#include <vix/json/Simple.hpp>
```

Main types:

-   `token` → generic JSON value
-   `array_t` → JSON array
-   `kvs` → JSON object (key/value storage)

------------------------------------------------------------------------

## token

``` cpp
#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  token t1;           // null
  token t2 = true;    // bool
  token t3 = 42;      // int64
  token t4 = 3.14;    // double
  token t5 = "hello"; // string

  std::cout << t2.as_bool_or(false) << "\n";
  std::cout << t3.as_i64_or(0) << "\n";
  std::cout << t5.as_string_or("") << "\n";

  return 0;
}
```

------------------------------------------------------------------------

## array_t

``` cpp
#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  array_t arr;

  arr.push_int(1);
  arr.push_string("two");
  arr.push_bool(true);

  for (std::size_t i = 0; i < arr.size(); ++i)
  {
    if (arr[i].is_string())
      std::cout << arr[i].as_string_or("") << "\n";
  }

  return 0;
}
```

------------------------------------------------------------------------

## kvs (object)

``` cpp
#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  kvs user;

  user.set_string("name", "Alice");
  user.set_int("age", 30);
  user.set_bool("active", true);

  std::cout << user.get_string_or("name", "") << "\n";

  return 0;
}
```

------------------------------------------------------------------------

## Nested structures

``` cpp
#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  kvs root;

  kvs& user = root.ensure_object("user");
  user.set_string("name", "Gaspard");

  array_t& skills = user.ensure_array("skills");
  skills.push_string("C++");
  skills.push_string("P2P");

  std::cout << skills[0].as_string_or("") << "\n";

  return 0;
}
```

------------------------------------------------------------------------

## Merge and erase

``` cpp
#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  kvs base = obj({"env", "prod", "debug", false});
  kvs override = obj({"debug", true});

  base.merge_from(override, true);
  base.erase("env");

  for (auto& k : base.keys())
    std::cout << k << "\n";

  return 0;
}
```

# 2) High-level JSON (nlohmann powered)

Include:

``` cpp
#include <vix/json/json.hpp>
```

Helpers:

-   `o()` → object builder
-   `a()` → array builder
-   `kv()` → object from initializer list
-   `dumps()` → stringify with indentation
-   `loads()` → parse string
-   `dump_file()` / `load_file()` → safe file IO

## Object + array builders

``` cpp
#include <vix/json/json.hpp>
#include <iostream>

int main()
{
  using namespace vix::json;

  auto j = o(
    "message", "Hello",
    "count", 3,
    "arr", a(1, 2, 3)
  );

  std::cout << dumps(j, 2) << "\n";

  return 0;
}
```

## File IO

``` cpp
#include <vix/json/json.hpp>
#include <iostream>

int main()
{
  using namespace vix::json;

  auto j = loads(R"({"a":1,"b":[10,20]})");

  dump_file("out.json", j, 2);

  auto j2 = load_file("out.json");

  std::cout << dumps(j2, 2) << "\n";

  return 0;
}
```

## JPath navigation

`jset()` and `jget()` allow nested mutation using path strings.

``` cpp
#include <vix/json/json.hpp>
#include <iostream>

int main()
{
  using namespace vix::json;

  Json j = obj();

  jset(j, "user.profile.name", "Gaspard");
  jset(j, "user.langs[2]", "cpp");

  if (auto v = jget(j, "user.langs[2]"))
    std::cout << v->get<std::string>() << "\n";

  std::cout << dumps(j, 2) << "\n";

  return 0;
}
```

# When to use which

Use Simple JSON when:

-   You want minimal overhead
-   You want type-safe state storage
-   You do not need advanced JSON features

Use High-level JSON when:

-   You build API responses
-   You want expressive builders
-   You need parsing and file IO

Both layers can coexist in the same application.


