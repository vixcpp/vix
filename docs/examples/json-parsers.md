# JSON Parsers Guide

Beginner-friendly guide to the JSON body parser middleware.

This middleware:
- Validates Content-Type
- Parses request body into nlohmann::json
- Stores parsed JSON into request state
- Returns structured errors automatically

## What the middleware does

When installed:

1) Checks body size (optional)
2) Validates Content-Type: application/json
3) Parses JSON
4) Stores result in:
```cpp
    req.state<middleware::parsers::JsonBody>()
```
## Minimal Example

Save as: json_app_simple.cpp

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/json", middleware::app::json_dev(
      256,   // max_bytes
      true,  // allow_empty
      true   // require_content_type
  ));

  app.post("/json", [](Request& req, Response& res)
  {
    auto& jb = req.state<middleware::parsers::JsonBody>();

    res.json({
      "ok", true,
      "received", jb.value
    });
  });

  app.run(8080);
}
```

Run:

```bash
vix run json_app_simple.cpp
```

## Test with curl

### Valid JSON

```bash
curl -i -X POST http://localhost:8080/json   -H "Content-Type: application/json"   --data '{"x":1}'
```

Result: 200 OK

### Invalid JSON

```bash
curl -i -X POST http://localhost:8080/json   -H "Content-Type: application/json"   --data '{"x":}'
```

Result: 400 invalid_json

### Wrong Content-Type

```bash
curl -i -X POST http://localhost:8080/json   -H "Content-Type: text/plain"   --data '{"x":1}'
```

Result: 415 unsupported_media_type

## Strict Mode Example

Disallow empty body:

```cpp
app.use("/json", middleware::app::json_dev(
    256,
    false,  // allow_empty = false
    true
));
```

Now:

Empty body -> 400 empty_body

## Error Codes

400 empty_body
400 invalid_json
413 payload_too_large
415 unsupported_media_type

## Production Tips

✔ Always keep require_content_type = true
✔ Set max_bytes to prevent abuse
✔ Combine with body_limit middleware globally
✔ Combine with CSRF for browser APIs

## Complete Copy-Paste Example

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::json_dev(
      512,
      false,
      true
  ));

  app.post("/api/data", [](Request& req, Response& res)
  {
    auto& jb = req.state<middleware::parsers::JsonBody>();

    if (!jb.value.contains("name"))
    {
      res.status(400).json({
        "ok", false,
        "error", "name is required"
      });
      return;
    }

    res.json({
      "ok", true,
      "name", jb.value["name"]
    });
  });

  app.run(8080);
}
```

Test:

```bash
curl -i -X POST http://localhost:8080/api/data   -H "Content-Type: application/json"   --data '{"name":"Gaspard"}'
```
You now understand how JSON parsing works internally in Vix.cpp.

