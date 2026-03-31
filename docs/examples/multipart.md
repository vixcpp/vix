# Multipart Uploads

This page gives small, copy-paste examples for **multipart/form-data** uploads in Vix.cpp.

It covers two related middlewares:

- `middleware::parsers::multipart()`
  Validates `Content-Type: multipart/form-data` + extracts the boundary (no parsing, no saving).
- `middleware::parsers::multipart_save()`
  Parses multipart parts and saves files to disk + collects text fields.

Each section has: one concept, one minimal `main()`, a quick curl test.

---

## 0) Mental model

A multipart request contains many parts:

- text fields: `name=value`
- file fields: `file=@path`

The middleware builds a `MultipartForm`:

- `form.fields["title"] = "..."`
- `form.files[i].saved_path = "uploads/..."`

The handler reads `req.state<MultipartForm>()`.

---

## 1) Minimal: save files + echo summary

Save as: `multipart_save_app_simple.cpp`

```cpp
/**
 *
 *  @file multipart_save_app_simple.cpp - Multipart save (minimal)
 *
 *  Run:
 *    vix run multipart_save_app_simple.cpp
 *
 *  Test:
 *    curl -i -X POST http://localhost:8080/upload \
 *      -F "title=hello" \
 *      -F "image=@./pic.png"
 *
 */
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // Save files to ./uploads and store MultipartForm in request state
  app.use("/upload", middleware::app::multipart_save_dev("uploads"));

  app.post("/upload", [](Request &req, Response &res)
  {
    auto &form = req.state<middleware::parsers::MultipartForm>();

    // Helper that converts MultipartForm to JSON (from presets.hpp)
    res.json(middleware::app::multipart_json(form, "uploads"));
  });

  app.get("/", [](Request &, Response &res)
  {
    res.send("POST /upload as multipart/form-data");
  });

  app.run(8080);
  return 0;
}
```

---

## 2) Limits: max request size, max files, max per-file size

`multipart_save()` has strict limits in `MultipartSaveOptions`:

- `max_bytes` total request body
- `max_files` number of file parts
- `max_file_bytes` size per file

Save as: `multipart_save_limits.cpp`

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/parsers/multipart_save.hpp>

using namespace vix;

int main()
{
  App app;

  middleware::parsers::MultipartSaveOptions opt{};
  opt.upload_dir = "uploads";
  opt.max_bytes = 2 * 1024 * 1024;      // 2 MB total
  opt.max_files = 2;                    // at most 2 files
  opt.max_file_bytes = 1 * 1024 * 1024; // 1 MB per file
  opt.keep_original_filename = false;   // safer default
  opt.keep_extension = true;

  app.use("/upload", middleware::app::adapt_ctx(
      middleware::parsers::multipart_save(opt)));

  app.post("/upload", [](Request &req, Response &res)
  {
    auto &form = req.state<middleware::parsers::MultipartForm>();
    res.json(middleware::app::multipart_json(form, "uploads"));
  });

  app.run(8080);
  return 0;
}
```

Expected errors:

- 413 `payload_too_large` if request too big
- 413 `too_many_files` if file count exceeds max
- 413 `file_too_large` if one file exceeds `max_file_bytes`
- 415 `unsupported_media_type` if not multipart/form-data
- 400 `missing_boundary` if boundary missing

---

## 3) Filename policy: safest defaults

Recommended defaults:

- `keep_original_filename=false` (generate safe unique names)
- `keep_extension=true` (keep `.png`, `.zip`, ...)

Save as: `multipart_save_filename_policy.cpp`

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/parsers/multipart_save.hpp>

using namespace vix;

int main()
{
  App app;

  middleware::parsers::MultipartSaveOptions opt{};
  opt.upload_dir = "uploads";
  opt.keep_original_filename = false; // recommended
  opt.keep_extension = true;          // keep .png, .jpg, ...
  opt.default_basename = "upload";    // upload_<epoch>_<rand>.ext

  app.use(middleware::app::adapt_ctx(middleware::parsers::multipart_save(opt)));

  app.post("/upload", [](Request &req, Response &res)
  {
    auto &form = req.state<middleware::parsers::MultipartForm>();
    res.json(middleware::app::multipart_json(form, "uploads"));
  });

  app.run(8080);
  return 0;
}
```

---

## 4) Read text fields + file paths (practical handler)

Save as: `multipart_profile_update.cpp`

```cpp
/**
 * Upload avatar + username in one request.
 *
 * Test:
 *   curl -i -X POST http://localhost:8080/profile \
 *     -F "username=gaspard" \
 *     -F "avatar=@./pic.png"
 */
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static std::string field_or_empty(
    const middleware::parsers::MultipartForm &f,
    const std::string &key)
{
  auto it = f.fields.find(key);
  return it == f.fields.end() ? "" : it->second;
}

int main()
{
  App app;

  app.use("/profile", middleware::app::multipart_save_dev("uploads"));

  app.post("/profile", [](Request &req, Response &res)
  {
    auto &form = req.state<middleware::parsers::MultipartForm>();

    const std::string username = field_or_empty(form, "username");

    std::string avatar_path;
    for (const auto &file : form.files)
    {
      if (file.field_name == "avatar")
      {
        avatar_path = file.saved_path;
        break;
      }
    }

    res.json({
      "ok", true,
      "username", username,
      "avatar_saved_path", avatar_path,
      "files_count", (long long)form.files.size()
    });
  });

  app.run(8080);
  return 0;
}
```

---

## 5) Lightweight probe: validate multipart only (no parsing, no saving)

Use `middleware::parsers::multipart()` when you only want header validation.

Save as: `multipart_probe.cpp`

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/parsers/multipart.hpp>

using namespace vix;

int main()
{
  App app;

  middleware::parsers::MultipartOptions opt{};
  opt.require_boundary = true;
  opt.max_bytes = 512 * 1024; // 512 KB
  opt.store_in_state = true;

  app.use("/probe", middleware::app::adapt_ctx(middleware::parsers::multipart(opt)));

  app.post("/probe", [](Request &req, Response &res)
  {
    auto &info = req.state<middleware::parsers::MultipartInfo>();
    res.json({
      "ok", true,
      "content_type", info.content_type,
      "boundary", info.boundary,
      "body_bytes", (long long)info.body_bytes
    });
  });

  app.run(8080);
  return 0;
}
```

---

## 6) curl cheatsheet

Upload file + fields:

```bash
curl -i -X POST http://localhost:8080/upload \
  -F "title=hello" \
  -F "image=@./pic.png"
```

Multiple files:

```bash
curl -i -X POST http://localhost:8080/upload \
  -F "images=@./a.png" \
  -F "images=@./b.png"
```

Force wrong content-type (should fail 415):

```bash
curl -i -X POST http://localhost:8080/upload \
  -H "Content-Type: text/plain" \
  --data "x"
```

---

## 7) Production tips

- Keep strict limits (`max_bytes`, `max_files`, `max_file_bytes`).
- Prefer generated names (`keep_original_filename=false`).
- Store only `saved_path` in DB (not raw body).
- Validate types after save:
  - check `file.content_type`
  - check extension
  - optionally verify magic bytes

---

## Summary

- Use `multipart_save()` to parse and save files.
- Use `multipart()` to validate multipart headers only.
- Read results from request state:
  - `req.state<MultipartForm>()`
  - `req.state<MultipartInfo>()`

