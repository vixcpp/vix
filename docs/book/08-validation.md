# Validation

In the previous chapter, you learned middleware.
Now you will learn validation.

Validation checks whether incoming data is correct before your application uses it.

```txt
request data → validation → business logic → response
```

## Why validation exists

A route that trusts input blindly can receive missing fields, invalid emails, weak passwords, wrong types, or unsafe data. Validation prevents bad input from reaching real application logic.

## Public header

```cpp
#include <vix/validation.hpp>
```

## Validate one string

```cpp
auto result = vix::validation::validate("email", email)
                  .required()
                  .email()
                  .length_max(120)
                  .result();

if (!result.ok())
{
  for (const auto &error : result.errors.all())
  {
    std::cout << "field=" << error.field << " message=" << error.message << "\n";
  }
}
```

## Common rules

| Rule            | Purpose                                   |
| --------------- | ----------------------------------------- |
| `required()`    | Requires a present and non-empty value.   |
| `email()`       | Requires a valid email address format.    |
| `length_min(n)` | Requires a string length of at least `n`. |
| `length_max(n)` | Requires a string length of at most `n`.  |
| `min(n)`        | Requires a numeric value of at least `n`. |
| `max(n)`        | Requires a numeric value of at most `n`.  |
| `between(a, b)` | Requires a value between `a` and `b`.     |
| `in_set({...})` | Requires one of the allowed values.       |

## Validate numbers

```cpp
int age = 17;
auto result = vix::validation::validate("age", age)
                  .min(18, "must be adult")
                  .max(120)
                  .result();
```

## Validate allowed values

```cpp
auto result = vix::validation::validate("role", role)
                  .required()
                  .in_set({"admin", "user", "guest"})
                  .result();
```

## Parsed validation (string → number)

```cpp
auto result = vix::validation::validate_parsed<int>("age", input)
                  .between(18, 120)
                  .result("age must be a number");
```

Useful for query params, route params, and form fields that arrive as strings.

## Schema validation

```cpp
struct UserInput
{
  std::string email;
  std::string password;

  static vix::validation::Schema<UserInput> schema()
  {
    return vix::validation::schema<UserInput>()
        .field("email", &UserInput::email,
               vix::validation::field<std::string>().required().email().length_max(120))

        .field("password", &UserInput::password,
               vix::validation::field<std::string>().required().length_min(8).length_max(64));
  }
};

UserInput input;
input.email = "bad-email";
input.password = "123";

auto result = UserInput::schema().validate(input);
```

## BaseModel

```cpp
struct RegisterForm : vix::validation::BaseModel<RegisterForm>
{
  std::string email;
  std::string password;

  static vix::validation::Schema<RegisterForm> schema()
  {
    return vix::validation::schema<RegisterForm>()
        .field("email", &RegisterForm::email,
               vix::validation::field<std::string>().required().email().length_max(120))

        .field("password", &RegisterForm::password,
               vix::validation::field<std::string>().required().length_min(8).length_max(64));
  }
};

RegisterForm form;
auto result = form.validate();            // call on object
auto result2 = RegisterForm::validate(form);  // static call
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

## Validation in a route

```cpp
static json::Json validation_errors_to_json(
    const vix::validation::ValidationErrors &errors)
{
  json::Json items = json::Json::array();
  for (const auto &error : errors.all())
  {
    items.push_back(json::kv({
        {"field", json::Json(error.field)},
        {"code", json::Json(vix::validation::to_string(error.code))},
        {"message", json::Json(error.message)},
    }));
  }
  return items;
}

template <typename Result>
static void respond_validation_error(Response &res, const Result &result)
{
  res.status(400).json(json::kv({
      {"ok", json::Json(false)},
      {"error", json::Json("validation failed")},
      {"errors", validation_errors_to_json(result.errors)}
  }));
}

app.post("/api/register", [](Request &req, Response &res){

  const auto &body = req.json();
  if (!body.is_object()) {
    res.status(400).json({
      "ok", false,
      "error", "expected JSON object body"
    });
    return;
  }

  RegisterInput input;
  input.email = body.value("email", "");
  input.password = body.value("password", "");

  auto result = input.validate();
  if (!result.ok()) {
    respond_validation_error(res, result);
    return;
  }

  res.status(201).json({
    "ok", true,
    "message", "registered"
  });

});
```

## Structured error shape

```json
{
  "ok": false,
  "error": "validation failed",
  "errors": [
    { "field": "email", "code": "format", "message": "invalid email format" },
    {
      "field": "password",
      "code": "length_min",
      "message": "password too short"
    }
  ]
}
```

## Common mistakes

### Validating after business logic

```cpp
// Wrong: create user then validate
// Correct: validate then create user
```

### Forgetting body shape check

```cpp
if (!body.is_object()) {
  respond_error(res, 400, "expected JSON object body");
  return;
}
```

### Forgetting to return after validation failure

```cpp
if (!result.ok()) {
  respond_validation_error(res, result);
  return;
}
```

### Returning only one validation error

For forms, return all field errors at once so users can fix everything together.

## What you should remember

The normal flow: Request → validation → business logic → Response.
Use single-value validation for simple fields, schemas for structs, structured errors for APIs.
The core idea: bad input should stop at the boundary of your application.

## Next chapter

[Next: Errors and logging](/book/09-errors-and-logging)
