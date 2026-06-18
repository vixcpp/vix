# Write JSON

The writing helpers convert JSON values to strings and write JSON documents to files.

They are designed for normal application JSON: API payloads, configuration files, metadata files, generated files, logs, fixtures, and local cache documents. The API stays small on purpose. You choose whether the output should be readable, compact, or written to disk.

```cpp id="e0sc4l"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json config = o(
    "app", "Vix.cpp",
    "debug", true,
    "port", 8080
  );

  vix::print(dumps_pretty(config));

  return 0;
}
```

## Header

For normal application code, include:

```cpp id="j3w728"
#include <vix/json.hpp>
```

For direct usage of the serialization API only, include:

```cpp id="mpgo6r"
#include <vix/json/dumps.hpp>
```

The writing helpers live in:

```cpp id="jdkp7j"
namespace vix::json
```

## Public API

| API                                           | Purpose                                         |
| --------------------------------------------- | ----------------------------------------------- |
| `dumps(json, indent, ensure_ascii)`           | Serialize JSON to a readable multi-line string. |
| `dumps_pretty(json, indent, ensure_ascii)`    | Explicit alias for readable JSON output.        |
| `dumps_compact(json, ensure_ascii)`           | Serialize JSON to a compact single-line string. |
| `dump_file(path, json, indent, ensure_ascii)` | Write a JSON document to a file.                |

The default indentation for readable output is two spaces.

## Pretty JSON with `dumps()`

Use `dumps()` when the output is meant to be read by a person.

```cpp id="mg9h7b"
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

  std::string text = dumps(user);

  vix::print(text);

  return 0;
}
```

Output shape:

```json id="jbqqpj"
{
  "id": 42,
  "name": "Ada",
  "roles": ["admin", "editor"]
}
```

`dumps()` is useful for configuration files, diagnostics, generated metadata, examples, and debugging output.

## Explicit pretty output with `dumps_pretty()`

`dumps_pretty()` does the same kind of readable serialization as `dumps()`, but the name makes the intent clearer at the call site.

```cpp id="hbu0fg"
std::string readable = dumps_pretty(data);
```

This is often better in documentation and application code because the function name says what kind of output is expected.

```cpp id="qy9w7d"
vix::print(dumps_pretty(data));
```

If you want four spaces instead of two, pass the indentation:

```cpp id="zxryii"
vix::print(dumps_pretty(data, 4));
```

## Compact JSON with `dumps_compact()`

Use `dumps_compact()` when the output is meant to be sent, stored, or logged as a single line.

```cpp id="ifpp7q"
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

```json id="moxcm9"
{ "type": "user.created", "user_id": 42, "ok": true }
```

Compact JSON is normally the right choice for network responses, single-line structured logs, cache entries, and machine-oriented payloads.

## Pretty vs compact output

Pretty JSON is easier to inspect manually.

```cpp id="c3rk58"
std::string readable = dumps_pretty(data);
```

Compact JSON is smaller and easier to embed into single-line protocols or logs.

```cpp id="rizz9a"
std::string payload = dumps_compact(data);
```

A practical rule is to use pretty output when a human will open the file or read the terminal output, and compact output when another program will consume the data.

## ASCII escaping

The serialization helpers accept an `ensure_ascii` option.

By default, it is `false`, so non-ASCII characters are kept as UTF-8.

```cpp id="rxy3pr"
Json data = o(
  "city", "Kampala",
  "message", "Bonjour"
);

std::string text = dumps_pretty(data);
```

When `ensure_ascii` is `true`, non-ASCII characters are escaped.

```cpp id="y3dq88"
std::string text = dumps_pretty(data, 2, true);
```

Use ASCII escaping only when you need compatibility with a system that cannot safely handle UTF-8.

## Write a JSON file with `dump_file()`

Use `dump_file()` to write a JSON document to disk.

```cpp id="d7hoxk"
#include <vix/json.hpp>

using namespace vix::json;

int main()
{
  Json config = o(
    "server", o(
      "host", "127.0.0.1",
      "port", 8080
    ),
    "log", o(
      "level", "info",
      "format", "kv"
    )
  );

  dump_file("config/app.json", config);

  return 0;
}
```

Generated file shape:

```json id="veeskj"
{
  "server": {
    "host": "127.0.0.1",
    "port": 8080
  },
  "log": {
    "level": "info",
    "format": "kv"
  }
}
```

The function creates parent directories on a best-effort basis. If the parent directory does not exist, it tries to create it before writing the file.

## How `dump_file()` writes

`dump_file()` writes through a temporary file before replacing the destination file.

The process is:

1. create parent directories when needed,
2. write the JSON document to `<path>.tmp`,
3. flush and close the temporary file,
4. replace the destination with the temporary file,
5. fall back to copy and remove if rename fails.

This approach reduces the chance of leaving a partially written destination file if the process fails while writing.

It does not turn JSON file writes into a full transactional storage system. It simply makes normal file replacement safer than writing directly into the final path.

## Write compact JSON to a file

`dump_file()` writes pretty JSON by default because files are often inspected by humans.

If you want compact JSON in a file, pass a compact indentation value through the underlying JSON serializer manually.

```cpp id="n0wsc4"
#include <fstream>
#include <vix/json.hpp>

using namespace vix::json;

int main()
{
  Json event = o(
    "type", "deploy.started",
    "ok", true
  );

  std::ofstream out("event.json", std::ios::binary | std::ios::trunc);
  out << dumps_compact(event);

  return 0;
}
```

Use this when the file is machine-oriented and you do not need pretty formatting.

For normal configuration and metadata files, prefer:

```cpp id="g21kdu"
dump_file("config/app.json", data);
```

## Write generated metadata

The writing helpers are useful when a program generates metadata during build, test, packaging, or runtime.

```cpp id="q888ts"
#include <vix/json.hpp>

using namespace vix::json;

int main()
{
  Json metadata = o(
    "name", "example",
    "runtime", "Vix.cpp",
    "modules", a("json", "http", "log"),
    "generated", true
  );

  dump_file(".vix/metadata.json", metadata);

  return 0;
}
```

Generated file shape:

```json id="jzqzun"
{
  "name": "example",
  "runtime": "Vix.cpp",
  "modules": ["json", "http", "log"],
  "generated": true
}
```

## Write API-style JSON strings

When a function needs to return a JSON string, build the value first and serialize it explicitly.

```cpp id="kdbnay"
#include <string>
#include <vix/json.hpp>

using namespace vix::json;

std::string make_health_payload()
{
  Json payload = o(
    "ok", true,
    "service", "api"
  );

  return dumps_compact(payload);
}
```

This keeps the JSON structure typed until the final serialization step.

Avoid building JSON manually with string concatenation:

```cpp id="y7738e"
std::string body = "{\"ok\":true,\"service\":\"api\"}";
```

Manual string construction becomes fragile once values contain quotes, backslashes, nested objects, arrays, or user-provided text.

## Use JSON with HTTP responses

In Vix HTTP handlers, you usually do not need to call `dumps_compact()` manually. Use `res.json(...)` and let the response layer serialize the payload.

```cpp id="ev0wq5"
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/health", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "service", "api"
    });
  });

  app.get("/metadata", [](Request &, Response &res) {
    using namespace vix::json;

    Json metadata = o(
      "name", "Vix.cpp",
      "module", "json"
    );

    res.json(metadata);
  });

  app.run();

  return 0;
}
```

Use `dumps_compact()` directly when you are not using `Response`, for example in a CLI tool, a custom transport, a cache value, or a file format that expects a JSON string.

## Write logs as JSON strings

For structured application logs, prefer the Vix log module when available. The JSON serialization helpers are still useful when you need to generate a JSON payload yourself.

```cpp id="w3344k"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json event = o(
    "level", "info",
    "message", "build completed",
    "duration_ms", 128
  );

  vix::print(dumps_compact(event));

  return 0;
}
```

Output:

```json id="fmlhsv"
{ "level": "info", "message": "build completed", "duration_ms": 128 }
```

For real application logging, use `vix::log` so log levels, formats, async behavior, and context are managed consistently by the logging system.

## Load after writing

A useful test for generated JSON is to write it, read it back, and validate the fields.

```cpp id="oj23ht"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json original = o(
    "name", "Vix.cpp",
    "ok", true
  );

  dump_file("output.json", original);

  Json reloaded = load_file("output.json");

  const std::string name = get_or<std::string>(reloaded, "name", "unknown");
  const bool ok = get_or<bool>(reloaded, "ok", false);

  vix::print("name", name);
  vix::print("ok", ok);

  return 0;
}
```

Output shape:

```txt id="tx6tbl"
name Vix.cpp
ok true
```

This pattern is useful in tests for code that generates configuration files or metadata documents.

## Error handling

`dumps()`, `dumps_pretty()`, and `dumps_compact()` return strings. They rely on the JSON value being serializable by `nlohmann::json`.

`dump_file()` can throw `std::runtime_error` if the temporary file cannot be opened, if writing fails, or if the destination replacement fails.

```cpp id="k7sljo"
try
{
  dump_file("config/app.json", data);
}
catch (const std::exception& e)
{
  vix::eprint("failed to write config:", e.what());
}
```

For required generated files, throwing is usually acceptable. For optional cache files or best-effort metadata files, catch the exception and continue with a degraded behavior.

## Large JSON documents

The writing helpers serialize the whole JSON document at once.

That is appropriate for configuration files, metadata, fixtures, normal API payloads, and local generated files. It is not designed for unbounded streaming output or very large data exports.

For large data exports, design the application around streaming or chunked output instead of building one large in-memory JSON document.

## Complete example

This example builds a JSON document, writes it to disk, reloads it, and prints both pretty and compact forms.

```cpp id="dsi35p"
#include <vix/json.hpp>
#include <vix/print.hpp>

using namespace vix::json;

int main()
{
  Json project = o(
    "name", "Vix.cpp",
    "kind", "runtime",
    "modules", a("core", "json", "http"),
    "settings", o(
      "debug", true,
      "port", 8080
    )
  );

  dump_file("build/project.json", project);

  Json reloaded = load_file("build/project.json");

  vix::print("Pretty:");
  vix::print(dumps_pretty(reloaded));

  vix::print("Compact:");
  vix::print(dumps_compact(reloaded));

  return 0;
}
```

Output shape:

```txt id="bbakvk"
Pretty:
{
  "name": "Vix.cpp",
  "kind": "runtime",
  "modules": [
    "core",
    "json",
    "http"
  ],
  "settings": {
    "debug": true,
    "port": 8080
  }
}
Compact:
{"name":"Vix.cpp","kind":"runtime","modules":["core","json","http"],"settings":{"debug":true,"port":8080}}
```

## Next steps

Continue with [Safe Access](/guides/json/safe-access) to learn how to read JSON fields without repeating manual checks and exception handling.
