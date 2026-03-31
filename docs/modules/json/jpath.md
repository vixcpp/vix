# JPath Guide

`vix/json/jpath.hpp` provides a minimal path language (JPath) to navigate and mutate JSON.

It is designed to be:
- small
- dependency-free (besides `nlohmann::json`)
- beginner-friendly
- practical for config trees and dynamic payload building

This is not a full JSONPath implementation. It is intentionally limited and predictable.

---

## Include

```cpp
#include <vix/json/jpath.hpp>
using namespace vix::json;
```

---

## What problem does JPath solve?

Without JPath, deep JSON access often looks like:

```cpp
auto name = j["user"]["profile"]["name"].get<std::string>();
```

This is verbose, and it throws easily when intermediate keys are missing.

With JPath:

```cpp
if (const Json* name = jget(j, "user.profile.name"))
{
  std::cout << name->get<std::string>() << "\n";
}
```

For writes, JPath can auto-create intermediate nodes:

```cpp
jset(j, "user.profile.name", "Ada");
jset(j, "user.roles[0]", "admin");
```

---

## Supported syntax

### 1) Dot notation

```text
"user.name"
"settings.theme"
```

### 2) Array indices

```text
"users[0].email"
"roles[1]"
```

### 3) Quoted keys inside brackets

Use this when the key contains dots, spaces, or characters that are not convenient in dot notation.

```text
["complex.key"].value
["a b c"][0]
```

Escapes supported in quoted keys:
- `\"` for a literal quote
- `\\` for a literal backslash

Example:

```text
["say \"hi\""].value
```

---

## Read vs Write behavior

This is the most important part.

### Read-only: `const Json* jget(const Json&, path)`

- Never throws
- Returns `nullptr` when path is invalid or missing

```cpp
Json j = R"({"settings":{"theme":"dark"}})"_json;

if (const Json* theme = jget(j, "settings.theme"))
{
  std::cout << theme->get<std::string>() << "\n";
}
else
{
  std::cout << "missing theme\n";
}
```

### Writable: `Json* jget(Json&, path)`

- May throw on syntax errors
- Auto-creates missing intermediate nodes

Creation rules:
- Missing object keys become `{}` objects
- Missing array indices grow the array and fill with nulls

```cpp
Json j = Json::object();

// Auto-creates: user -> object, roles -> array, then grows roles to [null, null]
Json* slot = jget(j, "user.roles[1]");
*slot = "editor";

std::cout << j.dump(2) << "\n";
```

Output will contain:

```json
{
  "user": {
    "roles": [
      null,
      "editor"
    ]
  }
}
```

### Safe write wrapper: `jset(Json&, path, value)`

- Returns `true` on success
- Returns `false` on any syntax/assignment error
- Never throws (errors are converted to `false`)

```cpp
Json j = Json::object();

bool ok = jset(j, "user.name", "Ada");
if (!ok)
{
  // invalid path
}
```

---

## Quick reference

### `tokenize_path(path) -> std::vector<Token>`

Tokenizes a path string into structured tokens.

- Throws `std::runtime_error` on invalid syntax

This is mostly useful if you want to parse once, then apply multiple operations (advanced use).

---

## Examples

### Read a nested value

```cpp
Json j = R"({
  "user": { "name": "Ada", "roles": ["admin", "editor"] },
  "settings": { "theme": "dark" }
})"_json;

if (const Json* theme = jget(j, "settings.theme"))
{
  std::cout << "theme=" << theme->get<std::string>() << "\n";
}
```

### Read array element

```cpp
if (const Json* role = jget(j, "user.roles[0]"))
{
  std::cout << role->get<std::string>() << "\n";
}
```

### Read a key that contains a dot

```cpp
Json j = R"({
  "meta": {
    "complex.key": { "value": 123 }
  }
})"_json;

const Json* v = jget(j, "meta["complex.key"].value");
```

### Write and auto-create

```cpp
Json j = Json::object();

jset(j, "user.name", "Ada");
jset(j, "user.address.city", "Kampala");
jset(j, "user.roles[1]", "developer"); // grows array

std::cout << j.dump(2) << "\n";
```

---

## Error cases and behavior

### Invalid syntax in read-only mode

Read-only `jget(const Json&, path)` returns `nullptr` if tokenization fails.

So this will not crash:

```cpp
const Json* p = jget(j, "user..name"); // nullptr
```

### Invalid syntax in writable mode

Writable `jget(Json&, path)` throws `std::runtime_error`.

So you should wrap it if the path is user-provided:

```cpp
try
{
  Json* p = jget(j, user_path);
  *p = 123;
}
catch (const std::exception& e)
{
  // invalid syntax
}
```

Or use `jset()` which already converts errors to `false`.

---

## Performance notes

- Tokenization is done with a lightweight parser, no regex.
- Array indices are parsed using `std::from_chars` (fast, locale-independent).
- The tokenizer reserves capacity using a segment count heuristic.

For hot paths, consider:
- parsing once with `tokenize_path()`
- reusing tokens in a loop (custom logic)

---

## When to use JPath

Use it when:
- JSON structure is deep and you want readable access
- you build dynamic payloads or config trees
- you want safe reads without chained `[]`
- you want writes that auto-create intermediate nodes

Do not use it when:
- you need full JSONPath features (filters, wildcards, recursive descent)
- you want strict schema validation (use `ensure()` style checks instead)

---

## Summary

JPath gives you a small, predictable path language for JSON:

- Read safely with `jget(const Json&, path)` returning `nullptr`
- Write with auto-create using `jget(Json&, path)`
- Write safely using `jset()` returning `true/false`
- Support for:
  - dot keys
  - `[index]` access
  - bracket quoted keys like `["complex.key"]`

