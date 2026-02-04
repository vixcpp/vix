#include <iostream>
#include <string>

#include <vix/validation/Validate.hpp>

using namespace vix::validation;

int main()
{
  std::string email = "john@example.com";

  auto res = validate("email", email)
                 .required()
                 .email()
                 .length_max(120)
                 .result();

  std::cout << "ok=" << res.ok() << "\n";
  return res.ok() ? 0 : 1;
}
