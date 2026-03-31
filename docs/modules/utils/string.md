# String helpers (vix::utils::String)

Small, header-only utilities for common string operations used across Vix.cpp:
trim, case transforms, prefix/suffix checks, split/join, and a few HTTP helpers
(boundary extraction, URL decode, query string parsing).

Source file: `String.hpp`

---

## Include

```cpp
#include <vix/utils/String.hpp>
```

Namespace:

```cpp
using namespace vix::utils;
```

---

## What this file provides

### Whitespace and trim (ASCII whitespace)

- `ltrim(std::string s) noexcept -> std::string`
- `rtrim(std::string s) noexcept -> std::string`
- `trim(std::string s) noexcept -> std::string`

These functions operate on a copy of the input string and return by value.
This is convenient for chaining:

```cpp
#include <vix/utils/String.hpp>

std::string cleaned()
{
  std::string s = "   hello  ";
  return vix::utils::trim(std::move(s)); // "hello"
}
```

Whitespace check is intentionally ASCII-focused via `_is_space()`:
space + `\t \n \v \f \r`.

---

## Case transform

### Lowercase (ASCII, C locale)

- `to_lower(std::string s) noexcept -> std::string`

This is byte-wise ASCII lowering. It is not Unicode case folding.

```cpp
#include <vix/utils/String.hpp>

std::string key_normalize(std::string s)
{
  return vix::utils::to_lower(vix::utils::trim(std::move(s)));
}
```

---

## Prefix and suffix checks

- `starts_with(std::string_view s, std::string_view p) noexcept -> bool`
- `ends_with(std::string_view s, std::string_view p) noexcept -> bool`

```cpp
#include <vix/utils/String.hpp>

bool is_json(std::string_view p)
{
  return vix::utils::ends_with(p, ".json");
}

bool is_vix_module(std::string_view name)
{
  return vix::utils::starts_with(name, "vix");
}
```

### Case-insensitive prefix

- `starts_with_icase(std::string_view s, std::string_view prefix) -> bool`

ASCII-only, predictable, fast.

```cpp
#include <vix/utils/String.hpp>

bool has_bearer(std::string_view header)
{
  return vix::utils::starts_with_icase(header, "bearer ");
}
```

---

## Split and join

### Split by a single character

- `split(std::string_view s, char sep) -> std::vector<std::string>`

Keeps empty segments.

```cpp
#include <vix/utils/String.hpp>

std::vector<std::string> parts()
{
  // ["a","b","","c"]
  return vix::utils::split("a,b,,c", ',');
}
```

### Split by a string separator

- `split(std::string_view s, std::string_view sep) -> std::vector<std::string>`

Keeps empty segments. If `sep` is empty, no split is performed (returns `{s}`).

```cpp
#include <vix/utils/String.hpp>

std::vector<std::string> parts()
{
  // ["a","b","","c"]
  return vix::utils::split("a--b----c", "--");
}
```

### Join

- `join(const std::vector<std::string>& v, std::string_view sep) -> std::string`

Pre-reserves capacity for fewer reallocations.

```cpp
#include <vix/utils/String.hpp>

std::string build()
{
  std::vector<std::string> v = {"a","b","","c"};
  return vix::utils::join(v, "::"); // "a::b::::c"
}
```

---

## HTTP helpers

### Extract multipart boundary

- `extract_boundary(std::string_view content_type) -> std::string`

Parses strings like:

- `multipart/form-data; boundary=----WebKitFormBoundaryabc`
- `multipart/form-data; boundary="----abc"`

```cpp
#include <vix/utils/String.hpp>

std::string boundary_from_header(std::string_view ct)
{
  return vix::utils::extract_boundary(ct);
}
```

### URL decode

- `url_decode(std::string_view in) -> std::string`

Rules:
- `+` becomes space
- `%XX` hex sequences are decoded
- invalid `%` sequences are left as-is

```cpp
#include <vix/utils/String.hpp>

std::string decode_demo()
{
  return vix::utils::url_decode("q=hello%20world+vix"); // "q=hello world vix"
}
```

### Query string parsing

- `parse_query_string(std::string_view qs) -> std::unordered_map<std::string, std::string>`

Parses key/value pairs separated by `&`. Keys and values are URL-decoded.
If a key appears multiple times, the last one wins (map overwrite).

```cpp
#include <vix/utils/String.hpp>

std::unordered_map<std::string, std::string> parse()
{
  // { "q": "hello world", "page": "2" }
  return vix::utils::parse_query_string("q=hello%20world&page=2");
}
```

---

## Notes and constraints

- All helpers are ASCII/byte-oriented by design (predictable, fast, no locale surprises).
- `split` keeps empty segments (this is intentional, useful for parsers).
- Query parsing uses `std::unordered_map`, so ordering is not preserved.

---

## File location suggestion (project)

If this is part of the utils module, keep it under:

```txt
modules/utils/include/vix/utils/String.hpp
```

