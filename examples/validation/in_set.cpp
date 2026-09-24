#include <iostream>
#include <string>
#include <vector>

#include <vix/validation/Validate.hpp>

using namespace vix::validation;

int main()
{
  std::string role = "admin";

  auto res = validate("role", role)
                 .required()
                 .in_set({"admin", "user", "guest"})
                 .result();

  std::cout << "ok=" << res.ok() << "\n";
  return res.ok() ? 0 : 1;
}
