#include <iostream>
#include <string>

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
                   .email()
                   .length_max(120))
        .field("password", &RegisterForm::password,
               vix::validation::field<std::string>()
                   .required()
                   .length_min(8)
                   .length_max(64));
  }
};

int main()
{
  RegisterForm f;
  f.email = "bad-email";
  f.password = "123";

  auto r = f.validate();

  std::cout << "ok=" << static_cast<bool>(r.ok()) << "\n";
  std::cout << "is_valid=" << static_cast<bool>(f.is_valid()) << "\n";

  if (!r.ok())
  {
    for (const auto &e : r.errors.all())
    {
      std::cout << " - field=" << e.field
                << " code=" << vix::validation::to_string(e.code)
                << " message=" << e.message << "\n";
    }
    return 1;
  }

  std::cout << "valid form\n";
  return 0;
}
