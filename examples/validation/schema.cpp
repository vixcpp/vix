#include <iostream>
#include <string>
#include <string_view>

#include <vix/validation/BaseModel.hpp>
#include <vix/validation/Schema.hpp>
#include <vix/validation/Validate.hpp>
#include <vix/validation/Pipe.hpp>

struct RegisterForm : vix::validation::BaseModel<RegisterForm>
{
  std::string email;
  std::string password;
  std::string age;

  RegisterForm(std::string e, std::string p, std::string a)
      : email(std::move(e)), password(std::move(p)), age(std::move(a))
  {
  }

  static vix::validation::Schema<RegisterForm> schema()
  {
    return vix::validation::schema<RegisterForm>()
        .field("email", &RegisterForm::email,
               [](std::string_view f, const std::string &v)
               {
                 return vix::validation::validate(f, v)
                     .required()
                     .email()
                     .length_max(120);
               })
        .field("password", &RegisterForm::password,
               [](std::string_view f, const std::string &v)
               {
                 return vix::validation::validate(f, v)
                     .required()
                     .length_min(8)
                     .length_max(64);
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
  RegisterForm form{"bad-email", "123", "abc"};

  auto res = form.validate();
  std::cout << "ok=" << res.ok() << "\n";
  std::cout << "errors=" << res.errors.size() << "\n";

  for (const auto &e : res.errors.all())
  {
    std::cout << " - field=" << e.field
              << " code=" << vix::validation::to_string(e.code)
              << " message=" << e.message
              << "\n";
  }

  return res.ok() ? 0 : 1;
}
