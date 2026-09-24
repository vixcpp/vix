#include <iostream>

#include <vix/conversion/ToString.hpp>

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

int main()
{
  {
    auto r = to_string(42);
    std::cout << "to_string(42) -> ";
    if (!r)
      print_err(r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = to_string(3.14);
    std::cout << "to_string(3.14) -> ";
    if (!r)
      print_err(r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = to_string(true);
    std::cout << "to_string(true) -> ";
    if (!r)
      print_err(r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = to_string(Role::Admin, roles);
    std::cout << "to_string(Role::Admin) -> ";
    if (!r)
      print_err(r.error());
    else
      std::cout << r.value() << "\n";
  }

  return 0;
}
