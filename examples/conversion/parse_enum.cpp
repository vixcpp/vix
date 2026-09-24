#include <iostream>
#include <string_view>

#include <vix/conversion/ToEnum.hpp>

using namespace vix::conversion;

enum class Role
{
  Admin,
  User,
};

static constexpr EnumEntry<Role> roles[] = {
    {"admin", Role::Admin},
    {"user", Role::User},
};

static void print_err(const ConversionError &e)
{
  std::cout << "error: " << to_string(e.code)
            << " position=" << e.position
            << " input='" << e.input << "'\n";
}

static const char *role_name(Role r)
{
  switch (r)
  {
  case Role::Admin:
    return "Admin";
  case Role::User:
    return "User";
  default:
    return "Unknown";
  }
}

int main()
{
  for (std::string_view s : {"admin", "USER", "guest"})
  {
    auto r = to_enum<Role>(s, roles); // case-insensitive default=true

    std::cout << "input='" << s << "' -> ";
    if (!r)
    {
      print_err(r.error());
      continue;
    }

    std::cout << "value=" << role_name(r.value()) << "\n";
  }

  return 0;
}
