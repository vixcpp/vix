# Request Validation (HTTP)

This example shows how to use `vix::validation` inside HTTP routes.

Each section: - minimal `main()` - validation inside a route -
structured JSON error response

------------------------------------------------------------------------

## 1) Validate JSON body (POST /register)

Validates a JSON payload using `BaseModel`.

``` cpp
#include <vix.hpp>
#include <vix/validation/BaseModel.hpp>
#include <vix/validation/Schema.hpp>

using namespace vix;
namespace J = vix::json;

struct RegisterForm : vix::validation::BaseModel<RegisterForm>
{
  std::string email;
  std::string password;

  static vix::validation::Schema<RegisterForm> schema()
  {
    using namespace vix::validation;

    return schema<RegisterForm>()
      .field("email", &RegisterForm::email,
        field<std::string>().required().email())
      .field("password", &RegisterForm::password,
        field<std::string>().required().length_min(8));
  }
};

int main()
{
  App app;

  app.post("/register", [](Request& req, Response& res)
  {
    RegisterForm form;

    const auto& j = req.json();
    if (j.contains("email")) form.email = j["email"].get<std::string>();
    if (j.contains("password")) form.password = j["password"].get<std::string>();

    auto result = form.validate();

    if (!result.ok())
    {
      J::array errors;
      for (const auto& e : result.errors.all())
      {
        errors.push_back(J::obj({
          "field", e.field,
          "message", e.message
        }));
      }

      res.status(400).json({
        "ok", false,
        "errors", errors
      });
      return;
    }

    res.json({
      "ok", true,
      "message", "User registered"
    });
  });

  app.run(8080);
  return 0;
}
```

Test:
```bash
    curl -X POST http://localhost:8080/register   -H "Content-Type: application/json"   -d '{"email":"bad","password":"123"}'
```
------------------------------------------------------------------------

## 2) Validate query parameters (GET /search)

Validate query inputs using `validate_parsed`.

``` cpp
#include <vix.hpp>
#include <vix/validation/Pipe.hpp>

using namespace vix;
using namespace vix::validation;

int main()
{
  App app;

  app.get("/search", [](Request& req, Response& res)
  {
    auto page = validate_parsed<int>("page",
                   req.query_value("page", "1"))
                   .min(1)
                   .max(100)
                   .result("page must be a number");

    if (!page.ok())
    {
      res.status(400).json({
        "ok", false,
        "error", page.errors.all().front().message
      });
      return;
    }

    res.json({
      "ok", true,
      "page", req.query_value("page", "1")
    });
  });

  app.run(8080);
  return 0;
}
```

Test:

    curl "http://localhost:8080/search?page=abc"

------------------------------------------------------------------------

## 3) Validate path parameter (GET /users/{id})

``` cpp
#include <vix.hpp>
#include <vix/validation/Pipe.hpp>

using namespace vix;
using namespace vix::validation;

int main()
{
  App app;

  app.get("/users/{id}", [](Request& req, Response& res)
  {
    auto id = validate_parsed<int>("id", req.param("id"))
                .min(1)
                .result("id must be a positive number");

    if (!id.ok())
    {
      res.status(400).json({
        "ok", false,
        "error", id.errors.all().front().message
      });
      return;
    }

    res.json({
      "ok", true,
      "id", req.param("id")
    });
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## 4) Form-style validation (POST /login)

Use `Form<T>` when binding raw input.

``` cpp
#include <vix.hpp>
#include <vix/validation/BaseModel.hpp>
#include <vix/validation/Schema.hpp>
#include <vix/validation/Form.hpp>

using namespace vix;

struct LoginForm : vix::validation::BaseModel<LoginForm>
{
  std::string email;
  std::string password;

  static vix::validation::Schema<LoginForm> schema()
  {
    using namespace vix::validation;

    return schema<LoginForm>()
      .field("email", &LoginForm::email,
        field<std::string>().required().email())
      .field("password", &LoginForm::password,
        field<std::string>().required().length_min(6));
  }
};

int main()
{
  App app;

  app.post("/login", [](Request& req, Response& res)
  {
    using Input = std::vector<std::pair<std::string_view, std::string_view>>;

    Input input = {
      {"email", req.query_value("email")},
      {"password", req.query_value("password")}
    };

    auto r = vix::validation::Form<LoginForm>::validate(input);

    if (!r)
    {
      res.status(400).json({
        "ok", false,
        "error", r.errors().all().front().message
      });
      return;
    }

    res.json({
      "ok", true,
      "message", "Login valid"
    });
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## Pattern Summary

Inside HTTP routes:

-   Bind request data
-   Call validation
-   If invalid → return 400 with structured errors
-   If valid → continue business logic

This keeps routing clean and validation explicit.

