# Performance

The `performance` group improves HTTP response efficiency.

It helps with:

```txt id="p4cyr3"
compressing dynamic route responses
adding ETags
returning 304 Not Modified
compressing eligible static file responses through a hook
```

This page is about response performance around HTTP routes.

It does not replace routing, static file serving, or caching.

Core still owns:

```txt id="d4n4rv"
routes
handlers
static files
templates
server lifecycle
```

The middleware module adds reusable performance behavior around responses.

## What performance provides

The performance group includes:

| Feature                  | Purpose                                                     |
| ------------------------ | ----------------------------------------------------------- |
| `compression()`          | Compress dynamic route responses when the client accepts it |
| `etag()`                 | Generate ETags for GET and HEAD responses                   |
| `static_compression.hpp` | Provide an optional static response compression hook        |

The main namespace is:

```cpp id="ijk12q"
namespace vix::middleware::performance
```

When using `vix::App`, use the App adapter when a direct App preset is not available:

```cpp id="fmzn1j"
vix::middleware::app::adapt_ctx(...)
```

## Performance vs HTTP cache

Performance middleware and HTTP cache solve different problems.

```txt id="zeljxp"
HTTP cache
  stores dynamic GET responses server-side
  can skip the handler on cache hit

compression
  reduces response body size

ETag
  lets clients revalidate a response
  can return 304 Not Modified

static response hook
  can compress files served by app.static_dir(...)
```

Use them together when appropriate, but keep their roles separate.

## Recommended order

A practical route stack can look like this:

```cpp id="krgo6p"
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::rate_limit_dev());

app.use("/api", middleware::app::http_cache({
  .ttl_ms = 30'000
}));

app.use("/api", middleware::app::adapt_ctx(
  middleware::performance::etag()
));

app.use("/api", middleware::app::adapt_ctx(
  middleware::performance::compression()
));
```

The idea is:

```txt id="zcejrx"
HTTP cache
  may replay a response and skip the handler

ETag
  can validate the final body

compression
  can reduce the final response body
```

The exact order depends on how your application writes responses and how you want to combine cache validation and compression.

Start simple.

Add one performance feature at a time, test headers, then combine them.

## Compression

`compression()` compresses dynamic route responses when the client sends an acceptable `Accept-Encoding` header.

It is useful for:

```txt id="t9yub7"
large JSON responses
HTML responses
text responses
API lists
generated documents
```

It should not be confused with static file compression.

This middleware runs in the normal App middleware chain.

```txt id="3nzjya"
request
  -> route middleware
  -> handler writes response
  -> compression middleware may compress response
  -> response
```

## Dynamic compression example

```cpp id="tfr2sa"
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use(middleware::app::adapt_ctx(
    middleware::performance::compression({
      .min_size = 8,
      .add_vary = true,
      .enabled = true
    })
  ));

  app.get("/", [](Request &, Response &res)
  {
    res.text("Compression middleware installed. Try /large.");
  });

  app.get("/large", [](Request &, Response &res)
  {
    res.status(200).text(std::string(2048, 'a'));
  });

  app.get("/small", [](Request &, Response &res)
  {
    res.status(200).text("small");
  });

  app.run(8080);
}
```

Run:

```bash id="vl6g6e"
vix run compression_demo.cpp
```

Request without compression:

```bash id="snla4s"
curl -i http://127.0.0.1:8080/large
```

Request with compression support:

```bash id="hugj00"
curl -i \
  http://127.0.0.1:8080/large \
  -H "Accept-Encoding: gzip"
```

Small response:

```bash id="vfgyhu"
curl -i \
  http://127.0.0.1:8080/small \
  -H "Accept-Encoding: gzip"
```

The small response may not be compressed because it is below `min_size`.

## Compression options

Main options:

| Option       | Purpose                                            |
| ------------ | -------------------------------------------------- |
| `min_size`   | Minimum body size before compression is considered |
| `add_vary`   | Add `Vary: Accept-Encoding`                        |
| `enabled`    | Enable or disable compression                      |
| `gzip_level` | Compression level when gzip support is available   |

Example:

```cpp id="qbjag7"
vix::middleware::performance::CompressionOptions opt;

opt.min_size = 1024;
opt.add_vary = true;
opt.enabled = true;

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::performance::compression(opt)
));
```

Use a minimum size to avoid wasting CPU on tiny responses.

A practical starting value is:

```txt id="spqb4d"
1024 bytes
```

## Vary header

When compression depends on `Accept-Encoding`, the response should include:

```txt id="ychs3w"
Vary: Accept-Encoding
```

This tells caches that the response can differ depending on the request's `Accept-Encoding` header.

Enable it with:

```cpp id="k2s1jc"
.add_vary = true
```

This is usually the correct default for compressed responses.

## When not to compress

Avoid compression for:

```txt id="hi3161"
very small responses
already compressed data
streaming responses
CPU-sensitive endpoints under heavy load
responses where latency matters more than bandwidth
```

Examples of already compressed data:

```txt id="z49j3x"
jpg
png
webp
zip
gz
mp4
```

For static files, prefer pre-compressed assets at the deployment layer when possible.

Use Vix static response compression when you want Vix itself to handle eligible static responses.

## ETag

`etag()` generates an entity tag for successful GET and HEAD responses.

The ETag lets a client ask:

```txt id="gk9n4a"
Has this response changed since the last time I fetched it?
```

If the client sends `If-None-Match` and the tag matches, the middleware can return:

```txt id="twk7tm"
304 Not Modified
```

This avoids sending the body again.

## ETag example

```cpp id="fnkbh8"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use(middleware::app::adapt_ctx(
    middleware::performance::etag({
      .weak = true,
      .add_cache_control_if_missing = false,
      .min_body_size = 1
    })
  ));

  app.get("/article", [](Request &, Response &res)
  {
    res.text("Hello from Vix");
  });

  app.run(8080);
}
```

Run:

```bash id="alnl37"
vix run etag_demo.cpp
```

First request:

```bash id="v7d8bo"
curl -i http://127.0.0.1:8080/article
```

Look for:

```txt id="rt9fix"
ETag: ...
```

Then revalidate with that value:

```bash id="tp1596"
curl -i \
  http://127.0.0.1:8080/article \
  -H 'If-None-Match: <etag-value>'
```

If the ETag matches, expected status:

```txt id="v5o15g"
304 Not Modified
```

## ETag options

Main options:

| Option                         | Purpose                                         |
| ------------------------------ | ----------------------------------------------- |
| `weak`                         | Generate weak ETags such as `W/"..."`           |
| `add_cache_control_if_missing` | Add a default `Cache-Control` header if missing |
| `cache_control`                | Cache-Control value to add when enabled         |
| `min_body_size`                | Minimum body size before adding an ETag         |

Example:

```cpp id="r9tc8x"
vix::middleware::performance::EtagOptions opt;

opt.weak = true;
opt.add_cache_control_if_missing = true;
opt.cache_control = "public, max-age=0";
opt.min_body_size = 1;

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::performance::etag(opt)
));
```

Use ETags when clients may repeatedly fetch the same response and can benefit from revalidation.

## ETag vs HTTP cache

ETag does not skip your route handler by itself in the same way server-side HTTP cache can.

ETag helps the client avoid downloading the body when it already has the latest version.

```txt id="hlnaeh"
HTTP cache middleware
  server stores response
  cache hit can skip handler

ETag middleware
  response gets a validation tag
  client can revalidate
  server can return 304
```

They can complement each other.

Use HTTP cache for server-side reuse.

Use ETag for client-side revalidation.

## Static response compression

Static file serving belongs to `vix::App`.

Use:

```cpp id="kod1mt"
app.static_dir(
  "public",
  "/",
  "index.html",
  true,
  "public, max-age=3600",
  true,
  false
);
```

Core handles:

```txt id="fvxgy8"
public directory
mount path
index file
Cache-Control
fallthrough
SPA fallback
file response
```

The middleware module can provide an optional hook after a static file response has been written.

That hook can compress eligible static file responses.

```cpp id="l37ol6"
vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook()
);
```

This is not route middleware.

It is a static response hook.

## Static compression with options

Use the same compression options when you want dynamic and static responses to follow the same compression policy.

```cpp id="8us72s"
vix::middleware::performance::CompressionOptions compression_options{
  .min_size = 1024,
  .add_vary = true,
  .enabled = true
};

app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::performance::compression(compression_options)
));

vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook(compression_options)
);

app.static_dir(
  "public",
  "/",
  "index.html",
  true,
  "public, max-age=3600",
  true,
  false
);
```

This does two different things:

```txt id="gc083j"
app.use(compression(...))
  compresses dynamic route responses

App::set_static_response_hook(...)
  compresses eligible static file responses served by app.static_dir(...)
```

Keep those responsibilities separate.

## Configuration-driven static compression

Generated applications may wire static behavior from environment or config values.

Example configuration:

```dotenv id="jm73yh"
PUBLIC_PATH=public
PUBLIC_MOUNT=/
PUBLIC_INDEX=index.html
PUBLIC_CACHE_CONTROL=public, max-age=3600
PUBLIC_SPA_FALLBACK=false
PUBLIC_COMPRESSION=false
PUBLIC_COMPRESSION_MIN_SIZE=1024
```

Bootstrap code can read those values and wire Core plus the optional middleware hook.

```cpp id="p17sn9"
const std::string publicPath = cfg.getString("public.path", "public");
const std::string publicMount = cfg.getString("public.mount", "/");
const std::string publicIndex = cfg.getString("public.index", "index.html");
const std::string publicCacheControl =
  cfg.getString("public.cache_control", "public, max-age=3600");

const bool publicSpaFallback = cfg.getBool("public.spa_fallback", false);
const bool publicCompression = cfg.getBool("public.compression", false);
const int publicCompressionMinSize =
  cfg.getInt("public.compression_min_size", 1024);

if (publicCompression)
{
  const auto compressionOptions =
    vix::middleware::performance::CompressionOptions{
      .min_size = static_cast<std::size_t>(publicCompressionMinSize),
      .add_vary = true,
      .enabled = true
    };

  app.use(vix::middleware::app::adapt_ctx(
    vix::middleware::performance::compression(compressionOptions)
  ));

  vix::App::set_static_response_hook(
    vix::middleware::performance::compressed_static_response_hook(compressionOptions)
  );
}

app.static_dir(
  publicPath,
  publicMount,
  publicIndex,
  true,
  publicCacheControl,
  true,
  publicSpaFallback
);
```

The model is:

```txt id="y37sdk"
Core reads static file configuration through app.static_dir(...)
Middleware contributes compression only when enabled
Bootstrap connects configuration to both
```

## Static files are not middleware

Do not describe static file serving as a middleware feature.

This is Core:

```cpp id="eymwdn"
app.static_dir(...)
```

This is middleware enhancement:

```cpp id="qq5pge"
vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook()
);
```

This is route middleware:

```cpp id="z9sft4"
app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::performance::compression()
));
```

The distinction matters:

```txt id="jkog6j"
app.static_dir(...)
  serves files

app.use(compression(...))
  compresses dynamic route responses

set_static_response_hook(...)
  can compress static file responses after Core writes them
```

## Compression and ETag together

Compression and ETag can both touch the response body or headers.

When combining them, test the actual headers returned by your application.

A simple starting point is:

```cpp id="v8h4ka"
app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::performance::etag()
));

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::performance::compression()
));
```

Then inspect:

```bash id="gg1i7s"
curl -i \
  http://127.0.0.1:8080/api/data \
  -H "Accept-Encoding: gzip"
```

Check:

```txt id="c2qf3t"
ETag
Vary
Content-Encoding
status code
body behavior
```

If your deployment already compresses responses at Nginx, CDN, or another proxy, avoid double compression.

Use one compression layer deliberately.

## Complete dynamic performance example

```cpp id="ywa7r6"
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());

  app.use("/api", middleware::app::adapt_ctx(
    middleware::performance::etag({
      .weak = true,
      .add_cache_control_if_missing = true,
      .cache_control = "public, max-age=0",
      .min_body_size = 1
    })
  ));

  app.use("/api", middleware::app::adapt_ctx(
    middleware::performance::compression({
      .min_size = 1024,
      .add_vary = true,
      .enabled = true
    })
  ));

  app.get("/api/data", [](Request &, Response &res)
  {
    res.status(200).text(std::string(4096, 'x'));
  });

  app.run(8080);
}
```

Run:

```bash id="xslq1u"
vix run performance_demo.cpp
```

Request normally:

```bash id="m7nfv2"
curl -i http://127.0.0.1:8080/api/data
```

Request with gzip support:

```bash id="vvox0h"
curl -i \
  http://127.0.0.1:8080/api/data \
  -H "Accept-Encoding: gzip"
```

Revalidate with ETag:

```bash id="yxbn5w"
curl -i \
  http://127.0.0.1:8080/api/data \
  -H 'If-None-Match: <etag-value>'
```

## Complete static compression example

```cpp id="da3pg3"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  vix::middleware::performance::CompressionOptions options{
    .min_size = 1024,
    .add_vary = true,
    .enabled = true
  };

  vix::App::set_static_response_hook(
    vix::middleware::performance::compressed_static_response_hook(options)
  );

  app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "public, max-age=3600",
    true,
    true
  );

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.run(8080);
}
```

This example only enables static response compression.

It does not install dynamic route compression.

To compress dynamic routes too, also add:

```cpp id="cvd4ps"
app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::performance::compression(options)
));
```

## Summary

Use the performance group to reduce repeated transfer cost and improve HTTP response behavior.

Use:

```cpp id="jdlwif"
compression()
```

for dynamic route responses.

Use:

```cpp id="nuck43"
etag()
```

for client revalidation.

Use:

```cpp id="va1doe"
compressed_static_response_hook()
```

to enhance static file responses served by Core.

Remember the separation:

```txt id="e2yvdz"
HTTP cache skips repeated dynamic handler work
Compression reduces body size
ETag enables client revalidation
Core serves static files
Middleware can enhance static responses
```
