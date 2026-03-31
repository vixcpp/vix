# Validation

The `vix::validation` module provides a declarative, composable validation system
for:

- Struct-based models
- Raw values
- Parsed values
- HTTP form-style inputs
- Custom rules

It is designed for:

- Explicit schemas
- Structured errors
- No exceptions
- Reusable rules
- Backend-ready validation

Main headers:

```cpp
#include <vix/validation/BaseModel.hpp>
#include <vix/validation/Schema.hpp>
#include <vix/validation/Validate.hpp>
#include <vix/validation/Pipe.hpp>
#include <vix/validation/Form.hpp>
```

---

# 1. Validating a simple value

Use `validate(field, value)` for single-field validation.

```cpp
#include <vix/validation/Validate.hpp>

using namespace vix::validation;

std::string email = "john@example.com";

auto res = validate("email", email)
               .required()
               .email()
               .length_max(120)
               .result();

if (!res.ok())
{
  for (const auto& e : res.errors.all())
    std::cout << e.field << ": " << e.message << "\n";
}
```

## Numeric example

```cpp
int age = 17;

auto res = validate("age", age)
               .min(18, "must be adult")
               .max(120)
               .result();
```

---

# 2. Parsed validation (string → typed value)

When input is a string but should represent a number:

```cpp
#include <vix/validation/Pipe.hpp>

using namespace vix::validation;

std::string input = "25";

auto res = validate_parsed<int>("age", input)
               .between(18, 120)
               .result("age must be a number");
```

This performs:

1. Parse string → int
2. Validate numeric rules
3. Return structured errors

---

# 3. Schema-based struct validation

Use `Schema<T>` when validating a full object.

```cpp
#include <vix/validation/Schema.hpp>

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

Validate:

```cpp
User u;
u.email = "bad-email";
u.password = "123";

auto r = User::schema().validate(u);

if (!r.ok())
{
  for (const auto& e : r.errors.all())
    std::cout << e.field << ": " << e.message << "\n";
}
```

---

# 4. BaseModel pattern

`BaseModel<T>` adds `.validate()` and `.is_valid()` helpers.

```cpp
#include <vix/validation/BaseModel.hpp>

struct RegisterForm : vix::validation::BaseModel<RegisterForm>
{
  std::string email;
  std::string password;

  static vix::validation::Schema<RegisterForm> schema()
  {
    return vix::validation::schema<RegisterForm>()
        .field("email", &RegisterForm::email,
               vix::validation::field<std::string>()
                   .required()
                   .email())
        .field("password", &RegisterForm::password,
               vix::validation::field<std::string>()
                   .required()
                   .length_min(8));
  }
};

RegisterForm f;
f.email = "bad-email";
f.password = "123";

auto r = f.validate();
```

---

# 5. Custom cross-field validation

Use `.check()` inside schema:

```cpp
.check([](const ResetPassword& obj,
          vix::validation::ValidationErrors& errors)
{
  if (obj.password != obj.confirm)
  {
    errors.add("confirm",
               vix::validation::ValidationErrorCode::Custom,
               "passwords do not match");
  }
})
```

Use this for:

- password confirmation
- business constraints
- multi-field validation logic

---

# 6. Validating HTTP-style form input

Use `Form<T>::validate(input)` where input is:

```cpp
std::vector<std::pair<std::string_view, std::string_view>>
```

Minimal example:

```cpp
using Input = std::vector<
  std::pair<std::string_view, std::string_view>>;

Input in = {
  {"email", "bad-email"},
  {"password", "123"},
};

auto r = vix::validation::Form<RegisterForm>::validate(in);

if (!r)
{
  for (const auto& e : r.errors().all())
    std::cout << e.field << ": " << e.message << "\n";
}
```

The Form system handles:

- binding raw input
- field validation
- parsed validation
- structured error reporting

---

# 7. Allowed values (in_set)

```cpp
auto res = validate("role", role)
               .required()
               .in_set({"admin", "user", "guest"})
               .result();
```

---

# 8. Optional values

```cpp
std::optional<int> score = std::nullopt;

auto res = validate("score", score)
               .rule(rules::required<int>("score is required"))
               .result();
```

---

# Error Model

Every validation result provides:

- `res.ok()`
- `res.errors.all()`
- Each error has:
  - `field`
  - `code`
  - `message`

Example:

```cpp
for (const auto& e : res.errors.all())
{
  std::cout << "field=" << e.field
            << " code=" << to_string(e.code)
            << " message=" << e.message
            << "\n";
}
```

---

# Recommended Usage

For backend systems:

- Use `Schema<T>` for domain models
- Use `BaseModel<T>` for DTO-style validation
- Use `validate_parsed<T>` for numeric input
- Use `.check()` for cross-field constraints
- Always inspect `res.ok()`

The API is explicit, composable, and suitable for
production-grade backend validation systems.


