# Validation (vix::utils::Validation)

Lightweight schema validation for `std::unordered_map<std::string, std::string>`.

This module is designed for form-like payloads or cases where you already have string values (query params, URL-encoded bodies, simple adapters from JSON). It is intentionally small and dependency-free.

## What you get

- Declarative `Schema` (field -> `Rule`)
- Aggregated errors: `FieldErrors` (field -> message)
- No exceptions for numeric parsing (`std::from_chars`)
- First-failure per field (required, length, numeric range, regex)

## API

### Types

```cpp
using FieldErrors = std::unordered_map<std::string, std::string>;
using Schema      = std::unordered_map<std::string, Rule>;
```

### Rule

```cpp
struct Rule {
  bool required = false;

  std::optional<std::size_t> min_len{};
  std::optional<std::size_t> max_len{};

  std::optional<long long> min{};
  std::optional<long long> max{};

  std::optional<std::regex> pattern{};

  std::string label; // used in error messages
};
```

### Builders

```cpp
vix::utils::Rule required(std::string label = "");
vix::utils::Rule len(std::size_t minL, std::size_t maxL, std::string lbl = "");
vix::utils::Rule num_range(long long minV, long long maxV, std::string lbl = "");
vix::utils::Rule match(std::string regex_str, std::string lbl = "");
```

### validate_map

```cpp
vix::utils::Result<void, vix::utils::FieldErrors>
validate_map(const std::unordered_map<std::string, std::string>& data,
             const vix::utils::Schema& schema);
```

Validation order per field:

1. required (present and non-empty)
2. length bounds (min_len, max_len)
3. numeric range (min, max) using base-10 `std::from_chars`
4. regex pattern using `std::regex_match` (full match)

If any field fails, you get `Err(FieldErrors)` with all failing fields. Otherwise `Ok()`.

## Quick start

```cpp
#include <vix/utils/Validation.hpp>
#include <iostream>

using vix::utils::Schema;
using vix::utils::validate_map;
using vix::utils::required;
using vix::utils::len;
using vix::utils::num_range;
using vix::utils::match;

int main() {
  std::unordered_map<std::string, std::string> data = {
    {"name", "Ada"},
    {"age", "21"},
    {"email", "ada@example.com"}
  };

  Schema schema = {
    {"name", required("Name")},
    {"age",  num_range(18, 120, "Age")},
    {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Email")}
  };

  auto res = validate_map(data, schema);

  if (res.is_err()) {
    const auto& errs = res.error();
    for (const auto& it : errs) {
      std::cerr << it.first << ": " << it.second << "\n";
    }
    return 1;
  }

  std::cout << "OK\n";
  return 0;
}
```

## Common patterns

### 1) Required + length

You can either keep it simple with `required()` only, or use `len()` to enforce bounds. If you want both, build the rule manually.

```cpp
#include <vix/utils/Validation.hpp>

vix::utils::Rule username_rule() {
  vix::utils::Rule r = vix::utils::required("Username");
  r.min_len = 3;
  r.max_len = 20;
  return r;
}
```

### 2) Optional field with length

If `required=false`, a missing or empty field is ignored.

```cpp
vix::utils::Rule bio_rule() {
  vix::utils::Rule r;
  r.label = "Bio";
  r.max_len = 160;
  return r;
}
```

### 3) Numeric range

`num_range(min, max)` checks that the string is a valid base-10 integer and within bounds.

```cpp
Schema schema = {
  {"age", vix::utils::num_range(18, 120, "Age")}
};
```

Notes:
- `from_chars` rejects trailing garbage, so `"21x"` fails.
- Leading `+` is not accepted by `from_chars` for signed integers in many standard libraries. Prefer `"21"` or `"-1"`.

### 4) Regex full match

`match()` uses `std::regex_match`, so the entire string must match the pattern.

```cpp
Schema schema = {
  {"slug", vix::utils::match(R"(^[a-z0-9]+(?:-[a-z0-9]+)*$)", "Slug")}
};
```

## Working with the returned errors

`FieldErrors` is a map of `field -> error_message`. You can convert that into a JSON response or show a friendly UI message.

Example: build a compact JSON error payload (manual stringify, no JSON library required):

```cpp
#include <vix/utils/Validation.hpp>
#include <string>

static std::string json_escape(std::string_view s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

static std::string errors_to_json(const vix::utils::FieldErrors& errs) {
  std::string out = "{\"errors\":{";
  bool first = true;
  for (const auto& it : errs) {
    if (!first) out += ",";
    first = false;
    out += "\"";
    out += json_escape(it.first);
    out += "\":\"";
    out += json_escape(it.second);
    out += "\"";
  }
  out += "}}";
  return out;
}
```

## Behavior details

### Empty string handling

A field is considered present only if:
- key exists in `data`
- value is not empty

So `"email": ""` behaves like missing.

### First error per field

For a given field, only the first failing rule is stored:
- if required fails, length or regex are not checked
- if length fails, numeric or regex are not checked
- etc

### Regex cost

`std::regex` can be heavier than other checks. This module stores `std::regex` inside the `Rule`, so you pay compilation once when you build the schema (not per validation call). For hot paths, consider reusing schemas instead of rebuilding them per request.

## Tips

- Build schemas once and reuse them for performance.
- Prefer `label` to produce user-facing messages.
- Keep patterns anchored (`^...$`) even though `regex_match` already matches whole string. It makes intent obvious.


