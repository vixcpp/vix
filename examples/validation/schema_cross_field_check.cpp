#include <iostream>
#include <string>

#include <vix/validation/Schema.hpp>
#include <vix/validation/ValidationErrors.hpp>
#include <vix/validation/ValidationError.hpp>

struct ResetPassword
{
  std::string password;
  std::string confirm;

  static vix::validation::Schema<ResetPassword> schema()
  {
    return vix::validation::schema<ResetPassword>()
        .field("password", &ResetPassword::password,
               vix::validation::field<std::string>()
                   .required()
                   .length_min(8)
                   .length_max(64))
        .field("confirm", &ResetPassword::confirm,
               vix::validation::field<std::string>()
                   .required()
                   .length_min(8)
                   .length_max(64))
        .check([](const ResetPassword &obj, vix::validation::ValidationErrors &errors)
               {
                 if (!obj.password.empty() && !obj.confirm.empty() && obj.password != obj.confirm)
                 {
                   errors.add("confirm",
                              vix::validation::ValidationErrorCode::Custom,
                              "passwords do not match");
                 } });
  }
};

int main()
{
  ResetPassword in;
  in.password = "password123";
  in.confirm = "password124";

  auto r = ResetPassword::schema().validate(in);

  std::cout << "ok=" << static_cast<bool>(r.ok()) << "\n";
  if (!r.ok())
  {
    for (const auto &e : r.errors.all())
    {
      std::cout << " - field=" << e.field << " code=" << vix::validation::to_string(e.code)
                << " message=" << e.message << "\n";
    }
    return 1;
  }

  std::cout << "valid reset\n";
  return 0;
}
