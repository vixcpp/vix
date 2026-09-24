# Authentication

This guide shows how to add authentication to a Vix REST API.

You will build:

- `POST /auth/register`
- `POST /auth/login`
- `GET /auth/me`
- protected routes
- bearer token style authentication

> This guide keeps the implementation simple and in-memory.
> For production, store users in SQLite or MySQL, hash passwords, and use real JWT/session middleware.

## Goal

```txt
client registers → client logs in → server returns token
→ client sends Authorization: Bearer <token> → server protects private routes
```

## Setup

```bash
vix new auth-api
cd auth-api
```

```cpp
#include <vix.hpp>
#include <vix/validation.hpp>
```

## Define the user model

```cpp
struct User
{
  int id{};
  std::string email;
  std::string password;
  std::string role{"user"};
};

struct AppState
{
  std::vector<User> users;
  std::unordered_map<std::string, int> tokens;
};
```

## Request body types

```cpp
struct RegisterBody
{
  std::string email;
  std::string password;
  std::string confirm;

  static vix::validation::Schema<RegisterBody> schema()
  {
    using namespace vix::validation;
    return vix::validation::schema<RegisterBody>()
        .field("email", &RegisterBody::email, field<std::string>().required("email is required").email("invalid email format").length_max(120))

        .field("password", &RegisterBody::password, field<std::string>().required("password is required").length_min(8).length_max(64))

        .field("confirm", &RegisterBody::confirm, field<std::string>().required("confirm is required"))

        .check([](const RegisterBody &body, ValidationErrors &errors){
          if (!body.password.empty() && !body.confirm.empty() && body.password != body.confirm)
            errors.add("confirm", ValidationErrorCode::Custom, "passwords do not match");
        });
  }
};

struct LoginBody : vix::validation::BaseModel<LoginBody>
{
  std::string email;
  std::string password;

  static vix::validation::Schema<LoginBody> schema()
  {
    using namespace vix::validation;
    return vix::validation::schema<LoginBody>()
        .field("email", &LoginBody::email, field<std::string>().required("email is required").email("invalid email format"))

        .field("password", &LoginBody::password, field<std::string>().required("password is required").length_min(8));
  }
};
```

## Helper functions

```cpp
// Generate a simple token (use secure random tokens in production)
static std::string make_token(int user_id)
{
  return "dev-token-" + std::to_string(user_id);
}

// Read bearer token from request
static std::string bearer_token_from_request(Request &req)
{
  const std::string header = req.header("Authorization");
  const std::string prefix = "Bearer ";
  if (header.rfind(prefix, 0) != 0) return "";
  return header.substr(prefix.size());
}

// Find current user from token
static User *current_user(AppState &state, Request &req)
{
  const std::string token = bearer_token_from_request(req);
  if (token.empty()) return nullptr;
  auto it = state.tokens.find(token);
  if (it == state.tokens.end()) return nullptr;
  for (auto &user : state.users)
    if (user.id == it->second) return &user;
  return nullptr;
}
```

## Auth routes

```cpp
// POST /auth/register
app.post("/auth/register", [&state](Request &req, Response &res){
  // read body, validate, check duplicate, create user, return 201
});

// POST /auth/login
app.post("/auth/login", [&state](Request &req, Response &res){
  / validate, check credentials, create token, return token
});

// GET /auth/me
app.get("/auth/me", [&state](Request &req, Response &res){
  User *user = current_user(state, req);
  if (user == nullptr) {
    respond_error(res, 401, "unauthorized");
    return;
  }

  res.json(json::kv({
        {"ok", json::Json(true)},
        {"user",  json::kv({
                    {"id", json::Json(user->id)},
                    {"email", json::Json(user->email)},
                    {"role", json::Json(user->role)}
                  })
        }
      })
  );
});
```

## Protecting routes

```cpp
// Route-level protection
User *user = current_user(state, req);
if (user == nullptr) {
  respond_error(res, 401, "unauthorized");
  return;
}

// Role-based protection
if (user->role != "admin") {
  respond_error(res, 403, "admin role required");
  return;
}
```

## Test the API

```bash
# Health
curl -i http://127.0.0.1:8080/health

# Register
curl -i -X POST http://127.0.0.1:8080/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"ada@example.com","password":"password123","confirm":"password123"}'

# Login
curl -i -X POST http://127.0.0.1:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email":"ada@example.com","password":"password123"}'

# Access protected route
curl -i http://127.0.0.1:8080/auth/me \
  -H "Authorization: Bearer dev-token-1"

# Access without token
curl -i http://127.0.0.1:8080/private
```

## Authentication status codes

| Status | Meaning       | Example                           |
| ------ | ------------- | --------------------------------- |
| `201`  | Created.      | User registered successfully.     |
| `400`  | Bad Request.  | Invalid input was provided.       |
| `401`  | Unauthorized. | Missing or invalid auth token.    |
| `403`  | Forbidden.    | User is authenticated but denied. |
| `409`  | Conflict.     | Email is already registered.      |

## Important production notes

For production: hash passwords, use secure random tokens, add token expiration, use HTTPS, use SQLite/MySQL persistence, rate limit login attempts.

## Common mistakes

### Forgetting `Authorization: Bearer`

```bash
# Wrong
curl -i http://127.0.0.1:8080/auth/me -H "Authorization: dev-token-1"

# Correct
curl -i http://127.0.0.1:8080/auth/me -H "Authorization: Bearer dev-token-1"
```

### Using 403 instead of 401

Use `401` when not authenticated. Use `403` when authenticated but not allowed.

### Forgetting to return after auth error

```cpp
// Wrong
if (user == nullptr) {
  respond_error(res, 401, "unauthorized");
}

res.json({"ok", true});

// Correct
if (user == nullptr) {
  respond_error(res, 401, "unauthorized");
  return;
}
```

## What to use next

- [Sessions guide](/guides/sessions)
- [Rate limiting guide](/guides/rate-limiting)
- [SQLite API guide](/guides/database/sqlite)
