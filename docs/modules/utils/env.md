# Env utilities

Header: `vix/utils/Env.hpp`

This module provides small helpers to read environment variables and convert them into useful types (`std::string`, `bool`, `int`, `unsigned`, `double`).

Goals:
- Header only
- No exceptions
- Safe defaults (missing or invalid values return the provided default)
- Whitespace tolerant (leading and trailing spaces are ignored)
- ASCII only case insensitive parsing for booleans

## API

### `env_or(key, def)`
Returns the variable value as a `std::string`, or `def` if missing.

```cpp
std::string host = vix::utils::env_or("APP_HOST", "127.0.0.1");
```

### `env_bool(key, def)`
Reads a variable as boolean.

Truthy values (case insensitive):
- `1`
- `true`
- `yes`
- `on`

Everything else is false. Missing key returns `def`.

```cpp
bool color = vix::utils::env_bool("VIX_COLOR", true);
```

### `env_int(key, def)`
Reads a signed integer in base 10 using `std::from_chars`.
- Trims whitespace
- Requires the full string to be parsed (no trailing garbage)

```cpp
int port = vix::utils::env_int("PORT", 8080);
```

### `env_uint(key, def)`
Reads an unsigned integer in base 10 using `std::from_chars`.
- Trims whitespace
- Requires full parse

```cpp
unsigned threads = vix::utils::env_uint("WORKERS", 4u);
```

### `env_double(key, def)`
Reads a `double` using `std::strtod`.
- Trims whitespace
- Requires full parse
- Uses `.` as decimal separator

```cpp
double ratio = vix::utils::env_double("CACHE_RATIO", 0.25);
```

## Platform notes

### `vix_getenv(name)`
Internally, the helpers call `vix_getenv`.

- POSIX: uses `std::getenv(name)`.
- Windows: uses `_dupenv_s` and stores the value in a `thread_local std::string`.
  - This avoids returning pointers to freed memory.
  - The returned `const char*` remains valid until the next `vix_getenv` call on the same thread.

Practical rule: if you need to keep the value, copy it (all public helpers already return by value).

## Behavior details

- Missing variable: returns the default.
- Empty value:
  - `env_or` returns an empty string (unless you passed a non empty default).
  - `env_bool` treats empty as false. If you want empty to behave like default, do not export the variable at all.
  - `env_int` and `env_uint` return `def`.
  - `env_double` returns `def`.
- Invalid numeric values (non digits, overflow, trailing characters): return `def`.

## Example patterns

### Feature flags

```cpp
const bool enable_metrics = vix::utils::env_bool("VIX_METRICS", false);
```

### Limits

```cpp
const unsigned max_conns = vix::utils::env_uint("VIX_MAX_CONNS", 1000u);
```

### Config selection

```cpp
const std::string config = vix::utils::env_or("VIX_CONFIG", "config.json");
```

## Common variables used in Vix runtime

These names are not required by the Env module, but they are commonly used by other Vix components.

- `NO_COLOR`: disable colors when set to any non empty value.
- `VIX_COLOR`: override coloring, values like `never`, `0`, `false`, `always`, `1`, `true`.
- `VIX_MODE`: values like `dev`, `watch`, `reload` map to dev mode, others map to run.
- `VIX_NO_HYPERLINK`: disable terminal hyperlinks when set.
- `VIX_NO_ANIM`: disable small banner animations when set.


