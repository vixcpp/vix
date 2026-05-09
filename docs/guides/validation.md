# Validation

This guide shows how to validate input in Vix.

You will learn how to validate:
- single values,
- query params,
- path params,
- JSON request bodies,
- forms,
- structs with schemas,
- and models with `BaseModel`.

## Why validation matters

A REST API should not trust input.
Clients can send missing fields, invalid emails, weak passwords, wrong types, invalid ids, or malformed form data.

A good API should return errors like this:

```json
{
  "ok": false,
  "error": "validation_failed",
  "errors": [
    { "field": "email", "code": "format", "message": "invalid email format" }
  ]
}
```

## Public header

```cpp
#include <vix/validation.hpp>
```

## Validate a single value

```cpp
#include <vix/validation.hpp>

auto result = vix::validation::validate("email", email)
                  .required()
                  .email()
                  .length_max(120)
                  .result();
```

## Common value rules

| Rule              | Purpose                                      |
|-------------------|----------------------------------------------|
| `required()`      | Requires a present and non-empty value.      |
| `email()`         | Requires a valid email address.              |
| `length_min(n)`   | Requires a string length of at least `n`.    |
| `length_max(n)`   | Requires a string length of at most `n`.     |
| `min(n)`          | Requires a numeric value of at least `n`.    |
| `max(n)`          | Requires a numeric value of at most `n`.     |
| `between(a, b)`   | Requires a numeric value between `a` and `b`.|
| `in_set(...)`     | Requires one of the allowed values.          |

## Read validation errors

```cpp
if (!result.ok())
{
  for (const auto &error : result.errors.all())
  {
    std::cout << "field=" << error.field << "\n";
    std::cout << "code=" << vix::validation::to_string(error.code) << "\n";
    std::cout << "message=" << error.message << "\n";
  }
}
```

## Validate parsed values

```cpp
auto result = vix::validation::validate_parsed<int>("age", input)
                  .between(18, 120)
                  .result("age must be a number");
```

## Schema validation

```cpp
struct User
{
  std::string email;
  std::string password;

  static vix::validation::Schema<User> schema()
  {
    return vix::validation::schema<User>()
        .field("email", &User::email,
               vix::validation::field<std::string>()
                   .required()
                   .email()
                   .length_max(120))

        .field("password", &User::password,
               vix::validation::field<std::string>()
                   .required()
                   .length_min(8)
                   .length_max(64));
  }
};
```

## Cross-field validation

```cpp
.check([](const ResetPassword &obj, vix::validation::ValidationErrors &errors){
  if (!obj.password.empty() && !obj.confirm.empty() && obj.password != obj.confirm){
    errors.add("confirm", vix::validation::ValidationErrorCode::Custom,
                "passwords do not match");
  }
});
```

## BaseModel

```cpp
struct LoginBody : vix::validation::BaseModel<LoginBody>
{
  std::string email;
  std::string password;

  static vix::validation::Schema<LoginBody> schema()
  {
    return vix::validation::schema<LoginBody>()
        .field("email", &LoginBody::email,
               vix::validation::field<std::string>()
                   .required("email is required")
                   .email("invalid email format"))

        .field("password", &LoginBody::password,
               vix::validation::field<std::string>()
                   .required("password is required")
                   .length_min(8, "password too short"));
  }
};

auto result = body.validate();
```

## Use validation in an HTTP API

### Build a consistent error response

```cpp
static void respond_validation_errors(
    Response &res,
    const vix::validation::ValidationErrors &errors)
{
  json::Json items = json::Json::array();
  for (const auto &error : errors.all())
  {
    items.push_back(json::kv({
        {"field", json::Json(std::string(error.field))},
        {"code", json::Json(std::string(vix::validation::to_string(error.code)))},
        {"message", json::Json(std::string(error.message))},
    }));
  }

  res.status(400).json(json::kv({
      {"ok", json::Json(false)},
      {"error", json::Json("validation_failed")},
      {"errors", std::move(items)},
  }));
}
```

### Validate JSON body route

```cpp
app.post("/register", [](Request &req, Response &res){
  const auto &j = req.json();

  RegisterBody body;
  body.email = json_string_or(j, "email");
  body.password = json_string_or(j, "password");
  body.confirm = json_string_or(j, "confirm");

  auto result = RegisterBody::schema().validate(body);
  if (!result.ok()) { respond_validation_errors(res, result.errors); return; }

  res.status(201).json(json::kv({
        {"ok", json::Json(true)},
        {"message", json::Json("registered")},
        {"email", json::Json(body.email)}
  }));
});
```

## Validate query params

```cpp
app.get("/register/check", [](Request &req, Response &res){
  RegisterBody body;
  body.email = req.query_value("email", "");
  body.password = req.query_value("password", "");
  body.confirm = req.query_value("confirm", "");

  auto result = RegisterBody::schema().validate(body);
  if (!result.ok()) {
    respond_validation_errors(res, result.errors);
    return;
  }

  res.json(json::kv({
    {"ok", json::Json(true)},
    {"email", json::Json(body.email)}
  }));
});
```

## Validate path params

```cpp
app.get("/users/{id}", [](Request &req, Response &res){
  const std::string id_text = req.param("id", "0");

  auto parsed = vix::validation::validate_parsed<int>("id", id_text)
                    .min(1, "id must be >= 1")
                    .result("id must be a number");

  if (!parsed.ok()) {
    respond_validation_errors(res, parsed.errors);
    return;
  }

  res.json(json::kv({
    {"ok", json::Json(true)},
    {"id", json::Json(std::stoi(id_text))}
  }));
});
```

## Common validation patterns

```cpp
// Required string
field<std::string>().required("name is required")

// Email
field<std::string>().required("email is required").email("invalid email format")

// Password
field<std::string>().required("password is required").length_min(8).length_max(64)

// Role
field<std::string>().required("role is required").in_set({"admin", "user", "guest"})

// Age from string
parsed<int>().between(18, 120, "age must be between 18 and 120").parse_message("age must be a number")
```

## When to use each validation tool

| Tool                      | Use when                                          |
|---------------------------|----------------------------------------------------|
| `validate(...)`           | You need to validate one value.                    |
| `validate_parsed<T>(...)` | You need to parse and validate a string.           |
| `Schema<T>`               | You need to validate a struct.                     |
| `BaseModel<T>`            | You want `body.validate()` or `T::validate(body)`. |
| `Form<T>`                 | You need to bind raw key-value input.              |

## Common mistakes

### Validating too late

```cpp
// Wrong
create_user(req.json());

// Correct
auto result = RegisterBody::schema().validate(body);
if (!result.ok()) {
  respond_validation_errors(res, result.errors);
  return;
}

create_user(body);
```

### Forgetting to return after validation error

```cpp
// Wrong
if (!result.ok()) {
  respond_validation_errors(res, result.errors);
}
res.json({"ok", true});

// Correct
if (!result.ok()) {
  respond_validation_errors(res, result.errors);
  return;
}
res.json({"ok", true});
```

## What to use next

- [Authentication guide](/guides/authentication)
- [SQLite API guide](/guides/sqlite-api)
- [REST API guide](/guides/build-rest-api)
