# Multipart Upload

This example shows how to handle `multipart/form-data` uploads with Vix middleware.

Use this when you need routes that receive:

```txt id="zj8k5y"
files
images
documents
form fields
mixed form data
```

The example uses:

```cpp id="jnw5yu"
#include <vix/middleware.hpp>
```

and the App preset:

```cpp id="br6h4f"
middleware::app::multipart_save_dev("uploads")
```

The middleware validates the multipart request, parses the form, saves uploaded files, and stores the parsed result in request state.

## What this example builds

The app exposes:

```txt id="la3tek"
GET  /
POST /upload
```

Uploaded files are saved into:

```txt id="t4pqss"
uploads/
```

The response returns JSON describing what was received.

## Project structure

Create:

```txt id="ub8eqe"
multipart_upload_demo/
├── multipart_upload.cpp
└── uploads/
```

Create the directory:

```bash id="qtt1oy"
mkdir -p multipart_upload_demo/uploads
cd multipart_upload_demo
touch multipart_upload.cpp
```

## Source

Open:

```txt id="uqw9aw"
multipart_upload.cpp
```

Add:

```cpp id="9uzj7t"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static void install_middleware(App &app)
{
  app.use("/upload", middleware::app::request_id_dev());
  app.use("/upload", middleware::app::timing_dev());
  app.use("/upload", middleware::app::security_headers_dev());

  app.use("/upload", middleware::app::body_limit_write_dev(
    10 * 1024 * 1024
  ));

  app.use("/upload", middleware::app::multipart_save_dev(
    "uploads"
  ));
}

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.send(
      "Multipart upload example\n"
      "\n"
      "Try:\n"
      "  curl -i -X POST http://127.0.0.1:8080/upload \\\n"
      "    -F \"title=My file\" \\\n"
      "    -F \"file=@README.md\"\n"
    );
  });

  app.post("/upload", [](Request &req, Response &res)
  {
    auto &form =
      req.state<middleware::parsers::MultipartForm>();

    res.json(middleware::app::multipart_json(form));
  });
}

int main()
{
  App app;

  install_middleware(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

## Run it

Run:

```bash id="a2g2sg"
vix run multipart_upload.cpp
```

The server listens on:

```txt id="cg8e1i"
http://127.0.0.1:8080
```

## Create a test file

Create a small file to upload:

```bash id="zq1bda"
echo "Hello from Vix multipart upload" > sample.txt
```

## Upload one file

```bash id="wgrs83"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -F "title=My first upload" \
  -F "file=@sample.txt"
```

Expected status:

```txt id="vxqpbf"
200 OK
```

Expected body shape:

```json id="agtilq"
{
  "fields": {
    "title": "My first upload"
  },
  "files": [
    {
      "field": "file",
      "filename": "sample.txt",
      "path": "uploads/...",
      "size": 32
    }
  ]
}
```

The exact file path and JSON shape can depend on the current multipart serialization helper, but the important result is:

```txt id="zldf0p"
the form field is parsed
the file is saved
the parsed multipart state is available in the handler
```

## Upload multiple fields

```bash id="es8sm3"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -F "title=Product image" \
  -F "category=electronics" \
  -F "description=Main product photo" \
  -F "file=@sample.txt"
```

The middleware parses normal form fields and file fields from the same request.

## Upload multiple files

Create another test file:

```bash id="f0eev8"
echo "Second file" > sample2.txt
```

Send both files:

```bash id="qneype"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -F "title=Multiple files" \
  -F "files=@sample.txt" \
  -F "files=@sample2.txt"
```

This is useful for product images, galleries, documents, and attachments.

## Test invalid content type

Send plain text instead of multipart:

```bash id="z1hgqs"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -H "Content-Type: text/plain" \
  --data "hello"
```

Expected status:

```txt id="r3q03x"
415 Unsupported Media Type
```

The multipart middleware expects:

```txt id="fr369h"
Content-Type: multipart/form-data
```

## Test missing boundary

Force a multipart content type without a boundary:

```bash id="hgrm12"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -H "Content-Type: multipart/form-data" \
  --data "x"
```

Expected status:

```txt id="fdzct5"
400 Bad Request
```

A valid multipart request needs a boundary.

When you use:

```bash id="cjxhxf"
-F "file=@sample.txt"
```

`curl` automatically creates the correct boundary.

## Test body limit

The example installs:

```cpp id="pzgji7"
app.use("/upload", middleware::app::body_limit_write_dev(
  10 * 1024 * 1024
));
```

This limits upload request bodies to roughly:

```txt id="m2u0ep"
10 MiB
```

To test a smaller limit, temporarily change it to:

```cpp id="msscbx"
app.use("/upload", middleware::app::body_limit_write_dev(
  1024
));
```

Then upload a larger file.

Expected status:

```txt id="im1fol"
413 Payload Too Large
```

Use body limits before multipart parsing so oversized requests are rejected early.

## How it works

The important middleware is:

```cpp id="w5wmjr"
app.use("/upload", middleware::app::multipart_save_dev(
  "uploads"
));
```

It does three things:

```txt id="b0f7cq"
validates multipart/form-data
saves uploaded files into uploads/
stores MultipartForm in request state
```

The route reads the parsed form from request state:

```cpp id="xoufz6"
auto &form =
  req.state<middleware::parsers::MultipartForm>();
```

Then it returns a JSON representation:

```cpp id="tx6h35"
res.json(middleware::app::multipart_json(form));
```

## Middleware order

The example installs middleware in this order:

```cpp id="fs494y"
app.use("/upload", middleware::app::request_id_dev());
app.use("/upload", middleware::app::timing_dev());
app.use("/upload", middleware::app::security_headers_dev());

app.use("/upload", middleware::app::body_limit_write_dev(
  10 * 1024 * 1024
));

app.use("/upload", middleware::app::multipart_save_dev(
  "uploads"
));
```

The order matters:

```txt id="jgw2k7"
request id
  identify the request

timing
  measure the request

security headers
  harden the response

body limit
  reject oversized uploads early

multipart parser
  parse fields and save files

handler
  uses MultipartForm
```

The body limit should run before the multipart parser.

## Why the parser is route-specific

The multipart parser is installed only on:

```cpp id="t50ozk"
app.use("/upload", ...)
```

not globally on:

```cpp id="mij67k"
app.use("/", ...)
```

That matters because most routes do not receive multipart bodies.

Good:

```cpp id="jyt84g"
app.use("/upload", middleware::app::multipart_save_dev("uploads"));
```

Risky:

```cpp id="npinn8"
app.use("/", middleware::app::multipart_save_dev("uploads"));
```

A parser should run only where that body format is expected.

## Multipart vs JSON

Use JSON for structured API payloads:

```txt id="udlyix"
application/json
```

Use multipart for uploads:

```txt id="ifd8rg"
multipart/form-data
```

A product creation route with images may use multipart because it sends both fields and files:

```txt id="g3kiff"
title
price
description
image files
```

A pure API route without files should usually use JSON.

## Add simple field validation

You can validate fields after reading `MultipartForm`.

Example shape:

```cpp id="s1i5yb"
app.post("/upload", [](Request &req, Response &res)
{
  auto &form =
    req.state<middleware::parsers::MultipartForm>();

  auto title = form.fields.find("title");

  if (title == form.fields.end() || title->second.empty())
  {
    res.status(422).json({
      "ok", false,
      "error", "Missing required field",
      "field", "title"
    });
    return;
  }

  res.json(middleware::app::multipart_json(form));
});
```

Use this pattern for required text fields.

## Production notes

For production uploads, add application-level checks.

At minimum:

```txt id="kzkmm9"
limit total body size
limit number of files
limit individual file size
validate file extension
validate MIME type
rename files safely
avoid trusting original filenames
store outside public directory when needed
scan files if required by your app
return stable file IDs instead of raw local paths
```

Do not trust client-provided filenames.

A client can send names with unsafe characters or misleading extensions.

The application should decide the final storage name.

## Upload directory

This example saves to:

```txt id="k7o6xz"
uploads/
```

For development, that is fine.

For production, prefer a configured path:

```dotenv id="rdg1x9"
UPLOADS_PATH=storage/uploads
```

Then wire it from config:

```cpp id="yc852x"
const std::string uploads_path =
  cfg.getString("uploads.path", "storage/uploads");

app.use("/upload", middleware::app::multipart_save_dev(
  uploads_path
));
```

Keep uploaded files out of source-controlled directories.

A typical `.gitignore` should include:

```txt id="di57y7"
uploads/
storage/uploads/
```

## Complete test flow

Run:

```bash id="tud6wv"
vix run multipart_upload.cpp
```

Create test files:

```bash id="dp1786"
echo "Hello from Vix" > sample.txt
echo "Second file" > sample2.txt
```

Upload one file:

```bash id="npapf4"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -F "title=One file" \
  -F "file=@sample.txt"
```

Upload two files:

```bash id="g5qz35"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -F "title=Two files" \
  -F "files=@sample.txt" \
  -F "files=@sample2.txt"
```

Invalid content type:

```bash id="pskkka"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -H "Content-Type: text/plain" \
  --data "hello"
```

Inspect uploaded files:

```bash id="wl7r04"
ls -la uploads/
```

## Summary

Use multipart upload when a route receives files.

The core setup is:

```cpp id="h6fp92"
app.use("/upload", middleware::app::body_limit_write_dev(
  10 * 1024 * 1024
));

app.use("/upload", middleware::app::multipart_save_dev(
  "uploads"
));
```

Then read the parsed form:

```cpp id="yd8hdf"
auto &form =
  req.state<middleware::parsers::MultipartForm>();
```

And return or process it:

```cpp id="djbk7l"
res.json(middleware::app::multipart_json(form));
```

The mental model is:

```txt id="w7f6bp"
body_limit
  protects the server

multipart_save_dev
  parses fields and saves files

handler
  validates application data and returns a response
```
