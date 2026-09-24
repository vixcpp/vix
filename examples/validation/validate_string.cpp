#include <iostream>
#include <string>

#include <vix/validation/Validate.hpp>

using namespace vix::validation;

int main()
{
  std::string email = "test@example.com"; // essaye "", "abc"

  auto res = validate("email", email)
                 .required("email is required")
                 .email("invalid email format")
                 .length_max(64, "email too long")
                 .result();

  std::cout << "ok = " << res.ok() << "\n";

  if (!res.ok())
  {
    for (const auto &e : res.errors.all())
    {
      std::cout << "- " << e.field << ": " << e.message << "\n";
    }
  }

  return res.ok() ? 0 : 1;
}
