# Form & Body Parsers Guide

Beginner-friendly guide for:

-   application/x-www-form-urlencoded
-   application/json
-   multipart/form-data

Each section includes: - Minimal server - curl tests - Expected behavior

# 1) Form Parser (application/x-www-form-urlencoded)

## Minimal example

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/form", middleware::app::form_dev(128));

  app.post("/form", [](Request &req, Response &res)
  {
    auto& fb = req.state<middleware::parsers::FormBody>();

    auto it = fb.fields.find("b");
    res.send(it == fb.fields.end() ? "" : it->second);
  });

  app.run(8080);
}
```

## Test

``` bash
curl -i -X POST http://localhost:8080/form \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "a=1&b=hello+world"
```

# 2) JSON Parser (application/json)

## Minimal example

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/json", middleware::app::json_dev(256, true, true));

  app.post("/json", [](Request &req, Response &res)
  {
    auto &jb = req.state<middleware::parsers::JsonBody>();
    res.json({
        "ok", true,
        "raw", jb.value.dump()
    });
  });

  app.run(8080);
}
```

## Test

``` bash
curl -i -X POST http://localhost:8080/json \
  -H "Content-Type: application/json" \
  --data '{"x":1}'
```

# 3) Multipart Parser (File Uploads)

## Minimal example

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/mp", middleware::app::multipart_save_dev("uploads"));

  app.post("/mp", [](Request &req, Response &res)
  {
    auto &form = req.state<middleware::parsers::MultipartForm>();
    res.json(middleware::app::multipart_json(form));
  });

  app.run(8080);
}
```

## Test

``` bash
curl -i -X POST http://localhost:8080/mp \
  -F "a=1" -F "file=@/etc/hosts"
```

Files are saved to ./uploads/

# Error Codes Summary

  Code   Meaning
  ------ --------------------------
  400    Invalid body
  413    Payload too large
  415    Unsupported Content-Type

# Recommended Usage

-   Use form_dev() for HTML forms
-   Use json_dev() for APIs
-   Use multipart_save_dev() for file uploads

Keep max_bytes small in production.

