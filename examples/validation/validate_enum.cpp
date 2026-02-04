#include <iostream>
#include <string>

#include <vix/validation/Pipe.hpp>
#include <vix/conversion/ToEnum.hpp>

using namespace vix::validation;
using namespace vix::conversion;

enum class Role
{
  Admin,
  User
};

static constexpr EnumEntry<Role> roles[] = {
    {"admin", Role::Admin},
    {"user", Role::User},
};

int main()
{
  std::string input = "admin"; // essaye "guest"

  auto parsed = parse_enum<Role>(input, roles);

  if (!parsed)
  {
    std::cout << "invalid role\n";
    return 1;
  }

  std::cout << "role parsed OK\n";
  return 0;
}
