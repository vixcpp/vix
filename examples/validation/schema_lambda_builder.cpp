#include <iostream>
#include <string>
#include <string_view>

#include <vix/validation/Schema.hpp>
#include <vix/validation/Validate.hpp>

struct Register
{
  std::string email;
  std::string password;

  static vix::validation::Schema<Register> schema()
  {
    return vix::validation::schema<Register>()
        .field("email", &Register::email,
               [](std::string_view f, const std::string &v)
               {
                 return vix::validation::validate(f, v)
                     .required()
                     .email()
                     .length_max(120);
               })
        .field("password", &Register::password,
               [](std::string_view f, const std::string &v)
               {
                 return vix::validation::validate(f, v)
                     .required()
                     .length_min(8)
                     .length_max(64);
               });
  }
};

int main()
{
  Register in;
  in.email = "hello@example.com";
  in.password = "short";

  auto r = Register::schema().validate(in);

  std::cout << "ok=" << static_cast<bool>(r.ok()) << "\n";
  if (!r.ok())
  {
    for (const auto &e : r.errors.all())
    {
      std::cout << " - field=" << e.field << " message=" << e.message << "\n";
    }
    return 1;
  }

  std::cout << "valid register\n";
  return 0;
}
