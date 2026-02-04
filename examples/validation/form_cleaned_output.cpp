#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <vix/validation/Form.hpp>
#include <vix/validation/Pipe.hpp>
#include <vix/validation/Validate.hpp>

struct UserClean
{
  std::string email;
  int age{0};
};

struct RegisterForm
{
  using cleaned_type = UserClean;

  std::string email;
  std::string age;

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

  UserClean clean() const
  {
    return UserClean{email, std::stoi(age)};
  }
};

int main()
{
  using Input = std::vector<std::pair<std::string_view, std::string_view>>;

  Input in = {
      {"email", "john@doe.com"},
      {"age", "25"},
  };

  auto r = vix::validation::Form<RegisterForm>::validate(in);
  if (!r)
  {
    for (const auto &e : r.errors().all())
      std::cout << " - " << e.field << ": " << e.message << "\n";
    return 1;
  }

  const auto &u = r.value();
  std::cout << "email=" << u.email << " age=" << u.age << "\n";
}
