# Templates

This guide shows how to render HTML templates with Vix.

## What you will build

```txt
GET /
GET /dashboard
GET /health
```

Template files:

```txt
views/
├── base.html
├── header.html
├── index.html
└── dashboard.html
```

## Setup

```bash
vix new templates-app
cd templates-app
mkdir -p views
```

## Configure the templates directory

```cpp
app.templates("./views");
```

## Minimal template app

`views/index.html`:

```html
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
      <title>{{ title }}</title>
    </head>
  <body>
    <h1>{{ title }}</h1>
    <p>Hello {{ user }}.</p>
  </body>
</html>
```

Route:

```cpp
app.get("/", [](Request &, Response &res)
{
  vix::template_::Context ctx;
  ctx.set("title", "Vix Templates");
  ctx.set("user", "Gaspard");
  res.render("index.html", ctx);
});
```

## Passing variables

```cpp
vix::template_::Context ctx;
ctx.set("title", "Dashboard");
ctx.set("user", "Ada");
ctx.set("logged_in", true);
ctx.set("total_orders", 42);
```

## Render a list

```cpp
vix::template_::Array features;
features.emplace_back("Blazing fast C++");
features.emplace_back("Template engine built-in");
ctx.set("features", features);
```

```html
<ul>
  {% for feature in features %}
    <li>{{ feature }}</li>
  {% endfor %}
</ul>
```

## Conditional rendering

```html
{% if logged_in %}
  <p>Welcome back, {{ user }}.</p>
{% else %}
  <p>Please log in.</p>
{% endif %}
```

## Layouts with extends

`views/base.html`:

```html
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
      <title>{{ title }}</title>
    </head>
  <body>
    <main>
      {% block content %}{% endblock %}
    </main>
  </body>
</html>
```

`views/index.html`:

```html
{% extends "base.html" %}
{% block content %}
  <h1>{{ title }}</h1>
  <p>Hello {{ user }}.</p>
{% endblock %}
```

## Include partials

`views/header.html`:

```html
<header>
  <strong>{{ app_name }}</strong>
  <nav>
    <a href="/">Home</a>
    <a href="/dashboard">Dashboard</a>
  </nav>
</header>
```

Usage:

```html
{% include "header.html" %}
```

## Objects in templates

```cpp
vix::template_::Object user;
user["id"] = 1;
user["name"] = "Ada";
user["role"] = "admin";
ctx.set("user", user);
```

```html
<p>{{ user.name }}</p>
<p>{{ user.role }}</p>
```

## Arrays of objects

```cpp
vix::template_::Array orders;
{
  vix::template_::Object order;
  order["id"] = "ORD-1001";
  order["customer"] = "Alice";
  order["status"] = "Paid";
  order["amount"] = 120;
  orders.emplace_back(order);
}
ctx.set("recent_orders", orders);
```

```html
{% for order in recent_orders %}
  <tr>
    <td>{{ order.id }}</td>
    <td>{{ order.customer }}</td>
    <td>{{ order.status }}</td>
    <td>{{ order.amount }}</td>
  </tr>
{% endfor %}
```

## Template features

| Feature    | Example                         |
|------------|---------------------------------|
| Variable   | `{{ title }}`                   |
| Condition  | `{% if logged_in %}`            |
| Loop       | `{% for item in items %}`       |
| Include    | `{% include "header.html" %}`   |
| Layout     | `{% extends "base.html" %}`     |
| Block      | `{% block content %}`           |

## Templates with database data

```cpp
app.get("/users", [&db](Request &, Response &res){
  vix::template_::Context ctx;
  vix::template_::Array users;

  auto rows = db.query("SELECT id, name FROM users");
  while (rows->next())
  {
    vix::template_::Object user;
    user["id"] = rows->row().getInt64(0);
    user["name"] = rows->row().getString(1);
    users.emplace_back(user);
  }

  ctx.set("users", users);
  res.render("users.html", ctx);
});
```

## Templates and static files

```cpp
app.static_dir("public");
```

```html
<link rel="stylesheet" href="/style.css" />
<script src="/app.js"></script>
```

## Common mistakes

### Forgetting `app.templates(...)`

```cpp
app.templates("./views");  // required before res.render(...)
```

### Putting all logic in the template

Keep calculations and database queries in C++.
Pass simple, ready-to-render values to the template.

### Forgetting layout variables

If `base.html` uses `{{ page_title }}`, every route using that layout must set `ctx.set("page_title", "...")`.

## Recommended structure

```txt
src/main.cpp
views/
├── base.html
├── header.html
└── dashboard.html
public/
├── style.css
└── app.js
```

## What to use next

- [Static files guide](/guides/static-files)
- [SQLite API guide](/guides/sqlite-api)
- [Production Nginx + systemd guide](/guides/production-nginx-systemd)
