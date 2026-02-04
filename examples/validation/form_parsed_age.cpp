#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <vix/validation/Form.hpp>
#include <vix/validation/Pipe.hpp>
#include <vix/validation/Validate.hpp>

struct RegisterForm
{
  std::string email;
  std::string age; // raw input

  static bool bind(
      RegisterForm &out,
      const std::vector<std::pair<std::string_view, std::string_view>> &in,
      vix::validation::ValidationErrors &errors)
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

    auto email = get("email");
    if (email.empty())
      errors.add({"email", vix::validation::ValidationErrorCode::Required, "email is required"});
    else
      out.email.assign(email);

    auto age = get("age");
    if (age.empty())
      errors.add({"age", vix::validation::ValidationErrorCode::Required, "age is required"});
    else
      out.age.assign(age);

    return errors.size() == 0;
  }

  static vix::validation::Schema<RegisterForm> schema()
  {
    return vix::validation::schema<RegisterForm>()
        .field("email", &RegisterForm::email,
               [](std::string_view f, const std::string &v)
               {
                 return vix::validation::validate(f, v).email();
               })
        .parsed<int>("age", &RegisterForm::age,
                     [](std::string_view f, std::string_view sv)
                     {
                       return vix::validation::validate_parsed<int>(f, sv)
                           .between(18, 120)
                           .result("age must be a number");
                     });
  }
};

int main()
{
  using Input = std::vector<std::pair<std::string_view, std::string_view>>;

  Input in = {
      {"email", "john@doe.com"},
      {"age", "abc"},
  };

  auto r = vix::validation::Form<RegisterForm>::validate(in);

  std::cout << "ok=" << static_cast<bool>(r) << "\n";
  if (!r)
  {
    for (const auto &e : r.errors().all())
    {
      std::cout << " - field=" << e.field << " message=" << e.message << "\n";
    }
  }
}
