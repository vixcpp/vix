# Form Parser

This example shows how to parse `application/x-www-form-urlencoded` request bodies with Vix middleware.

Use this when a route receives classic HTML form data such as:

```txt id="p4w7gr"
name=Gaspard&email=gaspard@example.com&message=Hello
```

This format is commonly used by:

```txt id="e9m7cz"
HTML forms
simple contact forms
login forms
search forms
small admin forms
browser-submitted forms
```

For file uploads, use multipart instead.

For JSON APIs, use the JSON parser.

## What this example builds

The app exposes:

```txt id="aqa2zv"
GET  /
POST /contact
```

The route `/contact` expects:

```txt id="gy09ws"
Content-Type: application/x-www-form-urlencoded
```

and fields:

```txt id="vix545"
name
email
message
```

The middleware parses the form and stores it in request state as:

```cpp id="wahq88"
middleware::parsers::FormBody
```

## Project structure

Create:

```txt id="pfabxs"
form_parser_demo/
└── form_parser.cpp
```

Create the file:

```bash id="akzzpz"
mkdir form_parser_demo
cd form_parser_demo
touch form_parser.cpp
```

## Source

Open:

```txt id="bc9nxx"
form_parser.cpp
```

Add:

```cpp id="pik98h"
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static std::string form_value(
  const middleware::parsers::FormBody &form,
  const std::string &key)
{
  auto it = form.fields.find(key);

  if (it == form.fields.end())
    return {};

  return it->second;
}

static bool missing(const std::string &value)
{
  return value.empty();
}

static void install_middleware(App &app)
{
  app.use("/contact", middleware::app::request_id_dev());
  app.use("/contact", middleware::app::timing_dev());
  app.use("/contact", middleware::app::security_headers_dev());

  app.use("/contact", middleware::app::body_limit_write_dev(
    16 * 1024
  ));

  app.use("/contact", middleware::app::form_dev(
    4096
  ));
}

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.send(
      "Form parser example\n"
      "\n"
      "Try:\n"
      "  curl -i -X POST http://127.0.0.1:8080/contact \\\n"
      "    -H \"Content-Type: application/x-www-form-urlencoded\" \\\n"
      "    --data \"name=Gaspard&email=gaspard@example.com&message=Hello\"\n"
    );
  });

  app.post("/contact", [](Request &req, Response &res)
  {
    auto &form =
      req.state<middleware::parsers::FormBody>();

    const std::string name = form_value(form, "name");
    const std::string email = form_value(form, "email");
    const std::string message = form_value(form, "message");

    if (missing(name))
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing required field",
        "field", "name"
      });
      return;
    }

    if (missing(email))
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing required field",
        "field", "email"
      });
      return;
    }

    if (missing(message))
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing required field",
        "field", "message"
      });
      return;
    }

    res.json({
      "ok", true,
      "received", true,
      "name", name,
      "email", email,
      "message", message
    });
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

```bash id="pc4db3"
vix run form_parser.cpp
```

The server listens on:

```txt id="rnnqvy"
http://127.0.0.1:8080
```

## Test the home route

```bash id="c7t9kk"
curl -i http://127.0.0.1:8080/
```

Expected body:

```txt id="a2uypo"
Form parser example
```

This route is public and does not use the form parser.

## Send a valid form

```bash id="d8e1cq"
curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "name=Gaspard&email=gaspard@example.com&message=Hello"
```

Expected status:

```txt id="t2zz0c"
200 OK
```

Expected body:

```json id="vgwvf9"
{
  "ok": true,
  "received": true,
  "name": "Gaspard",
  "email": "gaspard@example.com",
  "message": "Hello"
}
```

The form parser decodes the body and stores the fields in request state.

## URL decoding

The parser handles URL-encoded values.

Example:

```bash id="xk6uep"
curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "name=Gaspard+Kirira&email=gaspard%40example.com&message=Hello+from+Vix"
```

Expected decoded values:

```txt id="l5gm6u"
name
  Gaspard Kirira

email
  gaspard@example.com

message
  Hello from Vix
```

In URL-encoded forms:

```txt id="d1bn6y"
+
  becomes a space

%40
  becomes @
```

## Test missing field

Missing `message`:

```bash id="n8ny73"
curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "name=Gaspard&email=gaspard@example.com"
```

Expected status:

```txt id="mqx7jy"
422 Unprocessable Entity
```

Expected body:

```json id="jx93zm"
{
  "ok": false,
  "error": "Missing required field",
  "field": "message"
}
```

The middleware parses the HTTP body.

The handler validates application fields.

## Test wrong Content-Type

Send JSON to the form route:

```bash id="hqo4ie"
curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/json" \
  --data '{"name":"Gaspard"}'
```

Expected status:

```txt id="gxmbcz"
415 Unsupported Media Type
```

The form parser expects:

```txt id="t12ar5"
application/x-www-form-urlencoded
```

If a route expects JSON, use the JSON parser instead.

## Test payload too large

The example installs a broad body limit:

```cpp id="cmc9mr"
app.use("/contact", middleware::app::body_limit_write_dev(
  16 * 1024
));
```

and a form parser limit:

```cpp id="q65fh9"
app.use("/contact", middleware::app::form_dev(
  4096
));
```

To test the form parser limit:

```bash id="vz6ubo"
BIG="$(python3 -c 'print("name=Gaspard&email=gaspard@example.com&message=" + "x"*5000)')"

curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "$BIG"
```

Expected status:

```txt id="me7k0e"
413 Payload Too Large
```

Use body limits and parser limits together:

```txt id="a9rk5z"
body limit
  protects the route from large requests

parser limit
  protects the parser from large form bodies
```

## How it works

The key middleware is:

```cpp id="kj1x4u"
app.use("/contact", middleware::app::form_dev(
  4096
));
```

It checks:

```txt id="uazl0i"
Content-Type
body size
URL-encoded format
```

Then it stores:

```cpp id="cuxl2m"
middleware::parsers::FormBody
```

The route reads it with:

```cpp id="lz0p1i"
auto &form =
  req.state<middleware::parsers::FormBody>();
```

The fields are available in:

```cpp id="g7hdco"
form.fields
```

which is a map of:

```txt id="ps3mle"
string key
string value
```

## Middleware order

The example installs middleware in this order:

```cpp id="bkn64u"
app.use("/contact", middleware::app::request_id_dev());
app.use("/contact", middleware::app::timing_dev());
app.use("/contact", middleware::app::security_headers_dev());

app.use("/contact", middleware::app::body_limit_write_dev(
  16 * 1024
));

app.use("/contact", middleware::app::form_dev(
  4096
));
```

The order matters:

```txt id="ntbs16"
request id
  identify the request

timing
  measure the request

security headers
  harden the response

body limit
  reject oversized bodies early

form parser
  parse application/x-www-form-urlencoded

handler
  validates fields and returns response
```

The body limit should run before the parser.

## Why the parser is route-specific

The form parser is installed only on:

```cpp id="pi2qh6"
app.use("/contact", ...)
```

not on:

```cpp id="w9imf6"
app.use("/", ...)
```

That matters because most routes do not receive URL-encoded forms.

Good:

```cpp id="ydlpf4"
app.use("/contact", middleware::app::form_dev(4096));
```

Risky:

```cpp id="lejz3a"
app.use("/", middleware::app::form_dev(4096));
```

Install parsers only where the route expects that body format.

## HTML form example

You can send the same request from a browser form.

```html id="fvc9un"
<form method="post" action="/contact">
  <label>
    Name
    <input name="name" />
  </label>

  <label>
    Email
    <input name="email" type="email" />
  </label>

  <label>
    Message
    <textarea name="message"></textarea>
  </label>

  <button type="submit">Send</button>
</form>
```

By default, a normal HTML form like this sends:

```txt id="xl2fyz"
application/x-www-form-urlencoded
```

If the form includes file inputs, use:

```html id="hqofxy"
enctype="multipart/form-data"
```

and use the multipart middleware instead.

## Form parser vs JSON parser

Use form parser for classic HTML forms:

```txt id="mjzk8s"
application/x-www-form-urlencoded
```

Use JSON parser for API clients:

```txt id="ux39gy"
application/json
```

Use multipart parser for files:

```txt id="tbqofu"
multipart/form-data
```

Simple rule:

```txt id="l87aj8"
HTML form without files
  form parser

API request with JSON body
  JSON parser

HTML form with files
  multipart parser
```

## Add CORS for frontend apps

If a browser frontend runs on another origin, add CORS before the parser:

```cpp id="j3qf65"
app.use("/contact", middleware::app::cors_dev({
  "http://localhost:5173",
  "http://127.0.0.1:5173"
}));

app.use("/contact", middleware::app::body_limit_write_dev(
  16 * 1024
));

app.use("/contact", middleware::app::form_dev(
  4096
));
```

CORS should run before body parsers so preflight requests can be handled cleanly.

## Production notes

For production forms, add:

```txt id="gk5ctv"
CSRF protection for browser forms
rate limiting
server-side validation
email format validation
spam protection
logging
request id
clear error responses
```

For forms submitted by browsers, CSRF protection is important.

Example middleware shape:

```cpp id="n5rflj"
app.use("/contact", middleware::app::csrf_dev());
app.use("/contact", middleware::app::form_dev(4096));
```

The exact setup depends on how your frontend obtains and sends the CSRF token.

## Complete test flow

Run:

```bash id="h3c5yp"
vix run form_parser.cpp
```

Valid form:

```bash id="m1flam"
curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "name=Gaspard&email=gaspard@example.com&message=Hello"
```

URL-decoded form:

```bash id="vzpgh2"
curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "name=Gaspard+Kirira&email=gaspard%40example.com&message=Hello+from+Vix"
```

Missing field:

```bash id="xdh21m"
curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "name=Gaspard&email=gaspard@example.com"
```

Wrong content type:

```bash id="wmbps0"
curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/json" \
  --data '{"name":"Gaspard"}'
```

Large body:

```bash id="an9bqb"
BIG="$(python3 -c 'print("name=Gaspard&email=gaspard@example.com&message=" + "x"*5000)')"

curl -i \
  -X POST http://127.0.0.1:8080/contact \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "$BIG"
```

## Summary

Use the form parser when a route receives:

```txt id="s0vfvo"
application/x-www-form-urlencoded
```

The core setup is:

```cpp id="nwn8zm"
app.use("/contact", middleware::app::body_limit_write_dev(
  16 * 1024
));

app.use("/contact", middleware::app::form_dev(
  4096
));
```

Then read the parsed form:

```cpp id="df3qku"
auto &form =
  req.state<middleware::parsers::FormBody>();
```

Access fields:

```cpp id="rb1ibo"
auto name = form.fields["name"];
```

The mental model is:

```txt id="sbmfhr"
body_limit
  protects the route

form_dev
  parses URL-encoded fields

handler
  validates application data
```
