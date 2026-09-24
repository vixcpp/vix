# Templates

This page shows how to use templates with Vix Core.

Use it when you want to render HTML pages from route handlers using a template directory and a rendering context.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the app and template headers directly:

```cpp
#include <vix/app/App.hpp>
#include <vix/template/Context.hpp>
#include <vix/view/TemplateView.hpp>
```

## What templates provide

Templates let a Vix application generate HTML responses from files.

Use templates for:

- HTML pages
- dashboards
- server-rendered views
- admin panels
- forms
- emails or HTML fragments
- dynamic pages with variables

Templates are configured with `app.templates(...)`.

```cpp
app.templates("views");
```

Then handlers can call `res.render(...)`.

```cpp
res.render("index.html", ctx);
```

## Basic template setup

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.templates("views");

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    vix::tmpl::Context ctx;
    ctx.set("title", "Home");

    res.render("index.html", ctx);
  });

  app.run(8080);

  return 0;
}
```

Expected directory:

```text
views/
  index.html
```

## Public alias

Vix Core exposes the template namespace as:

```cpp
vix::tmpl
```

So you can write:

```cpp
vix::tmpl::Context ctx;
```

instead of referencing the internal template namespace directly.

## Configure the template directory

Use `templates(...)` on the app.

```cpp
app.templates("views");
```

This configures the root directory used to load template files.

Example:

```text
views/
  index.html
  about.html
  users/
    show.html
```

Then you can render:

```cpp
res.render("index.html", ctx);
res.render("about.html", ctx);
res.render("users/show.html", ctx);
```

## Render a template

Use `res.render(name, context)`.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::tmpl::Context ctx;
  ctx.set("title", "Welcome");

  res.render("index.html", ctx);
});
```

`name` is the template path relative to the template directory.

```text
views/index.html
```

is rendered with:

```cpp
res.render("index.html", ctx);
```

## Template context

The context stores values passed to the template.

```cpp
vix::tmpl::Context ctx;

ctx.set("title", "Home");
ctx.set("message", "Hello from Vix");
```

Then render:

```cpp
res.render("index.html", ctx);
```

## Render with route parameters

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.templates("views");

  app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
  {
    vix::tmpl::Context ctx;
    ctx.set("title", "User profile");
    ctx.set("user_id", req.param("id"));

    res.render("users/show.html", ctx);
  });

  app.run(8080);

  return 0;
}
```

Request:

```text
GET /users/42
```

Template path:

```text
views/users/show.html
```

## Render with query parameters

```cpp
app.get("/search", [](vix::Request &req, vix::Response &res)
{
  vix::tmpl::Context ctx;
  ctx.set("title", "Search");
  ctx.set("q", req.query_value("q", ""));

  res.render("search.html", ctx);
});
```

Request:

```text
GET /search?q=vix
```

## Render from middleware-provided state

Middleware can store data in the request state.

```cpp
struct CurrentUser
{
  std::string id;
  std::string name;
};
```

Store the user:

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)res;

  req.state().set(CurrentUser{"42", "Ada"});

  next();
});
```

Use it in a handler:

```cpp
app.get("/me", [](vix::Request &req, vix::Response &res)
{
  const auto &user = req.state().get<CurrentUser>();

  vix::tmpl::Context ctx;
  ctx.set("title", "My profile");
  ctx.set("user_id", user.id);
  ctx.set("user_name", user.name);

  res.render("me.html", ctx);
});
```

## Check if templates are configured

Use `has_views()`.

```cpp
if (app.has_views())
{
  vix::print("templates are configured");
}
```

This is useful for advanced setup code or plugins.

## Access the template view

Use `views()` to access the template rendering facade.

```cpp
auto &view = app.views();
```

If `templates(...)` was not called, `views()` throws.

```cpp
if (app.has_views())
{
  auto &view = app.views();
  (void)view;
}
```

Most applications do not need to access `views()` directly.

They use:

```cpp
res.render("index.html", ctx);
```

## Templates and Response

`res.render(...)` is part of the public response helper.

```cpp
res.render("index.html", ctx);
```

It uses the template view configured by `app.templates(...)`.

If templates are not configured, rendering fails.

Recommended:

```cpp
app.templates("views");

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::tmpl::Context ctx;
  ctx.set("title", "Home");

  res.render("index.html", ctx);
});
```

## Templates and static files

Templates and static files are different.

Use templates for dynamic HTML:

```cpp
app.templates("views");

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::tmpl::Context ctx;
  ctx.set("title", "Home");

  res.render("index.html", ctx);
});
```

Use static files for assets:

```cpp
app.static_dir("public/assets", "/assets");
```

Common structure:

```text
views/
  index.html

public/
  assets/
    app.css
    app.js
    logo.svg
```

## Template with assets

Example app:

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.templates("views");
  app.static_dir("public/assets", "/assets");

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    vix::tmpl::Context ctx;
    ctx.set("title", "Home");

    res.render("index.html", ctx);
  });

  app.run(8080);

  return 0;
}
```

Template can reference assets like:

```html
<link rel="stylesheet" href="/assets/app.css" />
<script src="/assets/app.js"></script>
```

## Template page example

Example template file:

```html
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>{{ title }}</title>
  </head>
  <body>
    <h1>{{ title }}</h1>
    <p>{{ message }}</p>
  </body>
</html>
```

Example handler:

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::tmpl::Context ctx;
  ctx.set("title", "Welcome");
  ctx.set("message", "Hello from Vix");

  res.render("index.html", ctx);
});
```

## Multiple pages

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.templates("views");

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    vix::tmpl::Context ctx;
    ctx.set("title", "Home");

    res.render("index.html", ctx);
  });

  app.get("/about", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    vix::tmpl::Context ctx;
    ctx.set("title", "About");

    res.render("about.html", ctx);
  });

  app.run(8080);

  return 0;
}
```

Directory:

```text
views/
  index.html
  about.html
```

## Templates in route groups

Templates work normally inside route groups.

```cpp
app.group("/admin", [](auto &admin)
{
  admin.get("/dashboard", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    vix::tmpl::Context ctx;
    ctx.set("title", "Dashboard");

    res.render("admin/dashboard.html", ctx);
  });
});
```

Route:

```text
GET /admin/dashboard
```

Template:

```text
views/admin/dashboard.html
```

## Templates with middleware

Middleware can prepare data for template handlers.

```cpp
struct PageInfo
{
  std::string app_name;
};
```

Middleware:

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)res;

  req.state().set(PageInfo{"Vix.cpp"});

  next();
});
```

Handler:

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  const auto &page = req.state().get<PageInfo>();

  vix::tmpl::Context ctx;
  ctx.set("title", "Home");
  ctx.set("app_name", page.app_name);

  res.render("index.html", ctx);
});
```

## Templates and errors

If a template cannot be rendered, the route handler may throw.

For user-facing apps, you can catch errors and return a response.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  try
  {
    vix::tmpl::Context ctx;
    ctx.set("title", "Home");

    res.render("index.html", ctx);
  }
  catch (const std::exception &e)
  {
    res.status(500).text("Template error");
  }
});
```

## Templates and JSON APIs

You can mix template pages and JSON APIs.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.templates("views");

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    vix::tmpl::Context ctx;
    ctx.set("title", "Home");

    res.render("index.html", ctx);
  });

  app.get("/api/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      {"status", "ok"}
    });
  });

  app.run(8080);

  return 0;
}
```

## Templates and redirects

A template handler can redirect instead of rendering.

```cpp
app.get("/dashboard", [](vix::Request &req, vix::Response &res)
{
  const bool logged_in = req.has_header("Authorization");

  if (!logged_in)
  {
    res.redirect("/login");
    return;
  }

  vix::tmpl::Context ctx;
  ctx.set("title", "Dashboard");

  res.render("dashboard.html", ctx);
});
```

## Templates and status codes

Set the status before rendering.

```cpp
app.get("/missing-page", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::tmpl::Context ctx;
  ctx.set("title", "Not found");

  res.status(404).render("404.html", ctx);
});
```

## Template lifecycle

The template lifecycle is:

```text
App::templates
  -> create FileSystemLoader
  -> create template Engine
  -> create TemplateView
  -> route handler builds Context
  -> ResponseWrapper::render
  -> TemplateView renders response
  -> Session writes HTTP response
```

## Recommended directory layout

```text
project/
  src/
    main.cpp
  views/
    index.html
    about.html
    errors/
      404.html
  public/
    assets/
      app.css
      app.js
      logo.svg
```

Example setup:

```cpp
app.templates("views");
app.static_dir("public/assets", "/assets");
```

## Complete example

```cpp
#include <vix.hpp>

#include <string>

struct CurrentUser
{
  std::string id;
  std::string name;
};

int main()
{
  vix::App app;

  app.templates("views");
  app.static_dir("public/assets", "/assets");

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    (void)res;

    req.state().set(CurrentUser{"42", "Ada"});

    next();
  });

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    vix::tmpl::Context ctx;
    ctx.set("title", "Home");
    ctx.set("message", "Hello from Vix");

    res.render("index.html", ctx);
  });

  app.get("/me", [](vix::Request &req, vix::Response &res)
  {
    const auto &user = req.state().get<CurrentUser>();

    vix::tmpl::Context ctx;
    ctx.set("title", "Profile");
    ctx.set("user_id", user.id);
    ctx.set("user_name", user.name);

    res.render("me.html", ctx);
  });

  app.get("/api/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      {"status", "ok"}
    });
  });

  app.run(8080);

  return 0;
}
```

Directory:

```text
views/
  index.html
  me.html

public/
  assets/
    app.css
```

## API summary

| API                           | Purpose                                  |
| ----------------------------- | ---------------------------------------- |
| `app.templates(directory)`    | Configure the template root directory.   |
| `app.has_views()`             | Return whether templates are configured. |
| `app.views()`                 | Access the template view facade.         |
| `vix::tmpl::Context`          | Store values passed to templates.        |
| `ctx.set(name, value)`        | Store a template variable.               |
| `res.render(name, context)`   | Render a template response.              |
| `app.static_dir(root, mount)` | Serve static assets used by templates.   |

## Best practices

Configure templates before routes that render views.

```cpp
app.templates("views");
```

Keep templates in a dedicated directory.

```text
views/
```

Keep assets in a public directory.

```text
public/assets/
```

Mount assets under `/assets`.

```cpp
app.static_dir("public/assets", "/assets");
```

Build the context explicitly in the handler.

```cpp
vix::tmpl::Context ctx;
ctx.set("title", "Home");
```

Return immediately after redirects or errors.

```cpp
if (!logged_in)
{
  res.redirect("/login");
  return;
}
```

Use JSON routes for APIs and templates for pages.

```cpp
app.get("/api/status", api_handler);
app.get("/", page_handler);
```

## Next steps

Read the next pages:

- [Static files](./static-files.md)
- [Response](./response.md)
- [Handlers](./handlers.md)
- [HTTP server](./http-server.md)
- [Configuration](./configuration.md)
