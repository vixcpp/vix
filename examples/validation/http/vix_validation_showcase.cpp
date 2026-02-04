/**
 *
 *  @file vix_validation_showcase.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp - Validation Showcase (examples/validation/)
 *
 *  Goal:
 *    One self-contained file showing the validation API style:
 *      - query params validation
 *      - path params + conversion
 *      - JSON body validation
 *      - Schema<T> with FieldSpec (no lambdas)
 *      - Schema<T> with lambdas (expert)
 *      - cross-field checks
 *      - BaseModel<T> (CRTP)
 *      - Form<T> (bind + schema + errors)
 *      - consistent HTTP 400 JSON error shape
 *
 *  Notes:
 *    - copy/paste friendly
 *    - main() has no business logic
 *
 *  Vix.cpp
 *
 */

// ============================================================================
// QUICK MAP
// ----------------------------------------------------------------------------
// GET   /validation/health
// GET   /validation/query/register?email=..&password=..&confirm=..
// GET   /validation/path/user/{id}
// POST  /validation/json/register
// POST  /validation/json/login
// POST  /validation/json/profile
// POST  /validation/form/kv/register
// GET   /validation/basemodel/demo
// ============================================================================

#include <vix.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <vix/conversion/Parse.hpp>

#include <vix/validation/BaseModel.hpp>
#include <vix/validation/Form.hpp>
#include <vix/validation/Schema.hpp>
#include <vix/validation/Validate.hpp>
#include <vix/validation/ValidationError.hpp>
#include <vix/validation/ValidationErrors.hpp>
#include <vix/validation/ValidationResult.hpp>

using namespace vix;
namespace J = vix::json;

// ---------------------------------------------------------------------------
// Helpers: consistent 400 error responses
// ---------------------------------------------------------------------------

static inline J::Json error_item_json(const vix::validation::ValidationError &e)
{
  // build.hpp: kv({{key, Json(...)}, ...})
  return J::kv({
      {"field", J::Json(std::string(e.field))},
      {"code", J::Json(std::string(vix::validation::to_string(e.code)))},
      {"message", J::Json(std::string(e.message))},
  });
}

static inline void respond_validation_errors(Response &res, const vix::validation::ValidationErrors &errors)
{
  J::Json items = J::Json::array();

  for (const auto &e : errors.all())
    items.push_back(error_item_json(e));

  res.status(400).json(J::kv({
      {"ok", J::Json(false)},
      {"error", J::Json("validation_failed")},
      {"errors", std::move(items)},
  }));
}

static inline void respond_conversion_error(Response &res,
                                            const vix::conversion::ConversionError &e,
                                            std::string field = "__input__")
{
  res.status(400).json(J::kv({
      {"ok", J::Json(false)},
      {"error", J::Json("conversion_failed")},
      {"field", J::Json(std::move(field))},
      {"code", J::Json(std::string(vix::conversion::to_string(e.code)))},
      {"position", J::Json(static_cast<long long>(e.position))},
      {"input", J::Json(std::string(e.input))},
  }));
}

// ---------------------------------------------------------------------------
// 1) Schema examples
// ---------------------------------------------------------------------------

struct RegisterBody
{
  std::string email;
  std::string password;
  std::string confirm;

  static vix::validation::Schema<RegisterBody> schema()
  {
    using namespace vix::validation;

    return vix::validation::schema<RegisterBody>()
        .field("email", &RegisterBody::email,
               field<std::string>()
                   .required("email is required")
                   .email("invalid email format")
                   .length_max(120, "email too long"))
        .field("password", &RegisterBody::password,
               field<std::string>()
                   .required("password is required")
                   .length_min(8, "password must be at least 8 characters")
                   .length_max(64, "password too long"))
        .field("confirm", &RegisterBody::confirm,
               field<std::string>()
                   .required("confirm is required")
                   .length_min(8, "confirm must be at least 8 characters")
                   .length_max(64, "confirm too long"))
        .check([](const RegisterBody &obj, ValidationErrors &out) -> void
               {
                 if (!obj.password.empty() && !obj.confirm.empty() && obj.password != obj.confirm)
                 {
                   out.add("confirm",
                           ValidationErrorCode::Custom,
                           "passwords do not match");
                 } });
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
        .field("email", &LoginBody::email,
               field<std::string>()
                   .required("email is required")
                   .email("invalid email format"))
        .field("password", &LoginBody::password,
               field<std::string>()
                   .required("password is required")
                   .length_min(8, "password too short"));
  }
};

struct ProfileBody
{
  std::string username;
  std::string age; // parsed via Schema::parsed<int>

  static vix::validation::Schema<ProfileBody> schema()
  {
    using namespace vix::validation;

    return vix::validation::schema<ProfileBody>()
        .field("username", &ProfileBody::username,
               field<std::string>()
                   .required("username is required")
                   .length_min(3, "username too short")
                   .length_max(24, "username too long"))
        .parsed<int>("age", &ProfileBody::age,
                     parsed<int>()
                         .between(13, 120, "age out of range")
                         .parse_message("age must be a number"));
  }
};

// ---------------------------------------------------------------------------
// 2) Form example: key/value binding + schema validation
// ---------------------------------------------------------------------------

struct RegisterForm
{
  std::string email;
  std::string password;

  using Input = std::vector<std::pair<std::string_view, std::string_view>>;

  static bool bind(RegisterForm &out, const Input &in, vix::validation::ValidationErrors &errors)
  {
    auto get = [&](std::string_view key) -> std::string_view
    {
      for (const auto &kv : in)
      {
        if (kv.first == key)
          return kv.second;
      }
      return {};
    };

    const auto email_sv = get("email");
    if (email_sv.empty())
      errors.add("email", vix::validation::ValidationErrorCode::Required, "email is required");
    else
      out.email.assign(email_sv);

    const auto pass_sv = get("password");
    if (pass_sv.empty())
      errors.add("password", vix::validation::ValidationErrorCode::Required, "password is required");
    else
      out.password.assign(pass_sv);

    return errors.ok();
  }

  static vix::validation::Schema<RegisterForm> schema()
  {
    using namespace vix::validation;

    return vix::validation::schema<RegisterForm>()
        .field("email", &RegisterForm::email,
               field<std::string>()
                   .required("email is required")
                   .email("invalid email format"))
        .field("password", &RegisterForm::password,
               field<std::string>()
                   .required("password is required")
                   .length_min(8, "password too short"));
  }
};

// ---------------------------------------------------------------------------
// 3) JSON helper for vix::json::Simple token
// ---------------------------------------------------------------------------

static inline std::string json_string_or(
    const J::Json &j,
    std::string_view key,
    std::string fallback = "")
{
  if (!j.is_object())
    return fallback;

  auto it = j.find(std::string(key));
  if (it == j.end() || !it->is_string())
    return fallback;

  return it->get<std::string>();
}

// ---------------------------------------------------------------------------
// 4) Routes
// ---------------------------------------------------------------------------

static void register_validation_routes(App &app)
{
  app.get("/validation/health", [](Request &, Response &res)
          { res.json(J::kv({
                {"ok", J::Json(true)},
                {"module", J::Json("validation")},
            })); });

  app.get("/validation/query/register", [](Request &req, Response &res)
          {
            RegisterBody body;
            body.email = req.query_value("email", "");
            body.password = req.query_value("password", "");
            body.confirm = req.query_value("confirm", "");

            auto vr = RegisterBody::schema().validate(body);
            if (!vr.ok())
            {
              respond_validation_errors(res, vr.errors);
              return;
            }

            res.json(J::kv({
              {"ok", J::Json(true)},
              {"email", J::Json(body.email)},
              {"hint", J::Json("validated from query params")},
            })); });

  app.get("/validation/path/user/{id}", [](Request &req, Response &res)
          {
            const std::string id_str = req.param("id", "0");

            auto idr = vix::conversion::parse<int>(id_str);
            if (!idr)
            {
              respond_conversion_error(res, idr.error(), "id");
              return;
            }

            const int id = idr.value();
            if (id <= 0)
            {
              vix::validation::ValidationErrors errors;
              errors.add("id", vix::validation::ValidationErrorCode::Min, "id must be >= 1");
              respond_validation_errors(res, errors);
              return;
            }

            res.json(J::kv({
              {"ok", J::Json(true)},
              {"id", J::Json(id)},
              {"hint", J::Json("parsed via conversion::parse<int>")},
            })); });

  app.post("/validation/json/register", [](Request &req, Response &res)
           {
             const auto &j = req.json();

             RegisterBody body;
             body.email = json_string_or(j, "email");
             body.password = json_string_or(j, "password");
             body.confirm = json_string_or(j, "confirm");

             auto vr = RegisterBody::schema().validate(body);
             if (!vr.ok())
             {
               respond_validation_errors(res, vr.errors);
               return;
             }

             res.status(201).json(J::kv({
               {"ok", J::Json(true)},
               {"message", J::Json("registered")},
               {"email", J::Json(body.email)},
             })); });

  app.post("/validation/json/login", [](Request &req, Response &res)
           {
             const auto &j = req.json();

             LoginBody body;
             body.email = json_string_or(j, "email");
             body.password = json_string_or(j, "password");

             auto vr = body.validate();
             if (!vr.ok())
             {
               respond_validation_errors(res, vr.errors);
               return;
             }

             res.json(J::kv({
               {"ok", J::Json(true)},
               {"message", J::Json("login ok")},
               {"email", J::Json(body.email)},
             })); });

  app.post("/validation/json/profile", [](Request &req, Response &res)
           {
             const auto &j = req.json();

             ProfileBody body;
             body.username = json_string_or(j, "username");
             body.age = json_string_or(j, "age");

             auto vr = ProfileBody::schema().validate(body);
             if (!vr.ok())
             {
               respond_validation_errors(res, vr.errors);
               return;
             }

             res.json(J::kv({
               {"ok", J::Json(true)},
               {"message", J::Json("profile ok")},
               {"username", J::Json(body.username)},
               {"age_raw", J::Json(body.age)},
             })); });

  app.post("/validation/form/kv/register", [](Request &req, Response &res)
           {
             const auto &j = req.json();

             const std::string email = json_string_or(j, "email");
             const std::string password = json_string_or(j, "password");

             RegisterForm::Input input;
             input.reserve(2);
             input.push_back({"email", std::string_view(email)});
             input.push_back({"password", std::string_view(password)});

             auto r = vix::validation::Form<RegisterForm>::validate(input);
             if (!r)
             {
               respond_validation_errors(res, r.errors());
               return;
             }

             res.status(201).json(J::kv({
               {"ok", J::Json(true)},
               {"message", J::Json("registered via Form")},
               {"email", J::Json(r.value().email)},
             })); });

  app.get("/validation/basemodel/demo", [](Request &req, Response &res)
          {
            LoginBody body;
            body.email = req.query_value("email", "");
            body.password = req.query_value("password", "");

            auto vr = LoginBody::validate(body);
            if (!vr.ok())
            {
              respond_validation_errors(res, vr.errors);
              return;
            }

            res.json(J::kv({
              {"ok", J::Json(true)},
              {"message", J::Json("BaseModel static validate ok")},
              {"email", J::Json(body.email)},
            })); });
}

// ---------------------------------------------------------------------------
// Bootstrap
// ---------------------------------------------------------------------------

static int run_server()
{
  App app;
  register_validation_routes(app);
  app.run(8080);
  return 0;
}

int main()
{
  return run_server();
}
