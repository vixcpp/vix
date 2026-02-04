#include <iostream>
#include <string>

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

int main()
{
  User u;
  u.email = "bad-email";
  u.password = "123";

  auto r = User::schema().validate(u);

  std::cout << "ok=" << static_cast<bool>(r.ok()) << "\n";
  if (!r.ok())
  {
    for (const auto &e : r.errors.all())
    {
      std::cout << " - field=" << e.field << " message=" << e.message << "\n";
    }
    return 1;
  }

  std::cout << "valid user\n";
  return 0;
}
