/**
 *
 *  @file vix_conversion_showcase.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp - Conversion Showcase (examples/conversion/)
 *
 *  Goal:
 *    A single, self-contained file that showcases how conversions look in Vix.cpp:
 *      - parse int / float / bool / enum from string_view
 *      - generic parse<T>
 *      - to_string for int / float / bool / enum
 *      - consistent error printing (code, position, input)
 *      - copy/paste friendly patterns for real projects (CLI, config, APIs)
 *
 *  Notes:
 *    - This file focuses on the public conversion API style.
 *    - No exceptions, no iostream parsing, errors as values (expected).
 *    - main() contains no business logic: it only calls functions.
 *
 *  Vix.cpp
 *
 */

// ============================================================================
// QUICK MAP (console demo)
// ----------------------------------------------------------------------------
// 1) integers:     to_int<T>
// 2) floats:       to_float<T>
// 3) bool:         to_bool
// 4) enum:         to_enum<T> (mapping table)
// 5) generic:      parse<T>
// 6) to_string:    int / float / bool / enum
// ============================================================================

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <vix/conversion/ConversionError.hpp>
#include <vix/conversion/Parse.hpp>
#include <vix/conversion/ToBool.hpp>
#include <vix/conversion/ToEnum.hpp>
#include <vix/conversion/ToFloat.hpp>
#include <vix/conversion/ToInt.hpp>
#include <vix/conversion/ToString.hpp>

using namespace vix::conversion;

// ---------------------------------------------------------------------------
// Error helper
// ---------------------------------------------------------------------------

static void print_err(std::string_view label, const ConversionError &e)
{
  std::cout << label << " -> error: " << to_string(e.code)
            << " position=" << e.position
            << " input='" << e.input << "'\n";
}

// ---------------------------------------------------------------------------
// Enum demo
// ---------------------------------------------------------------------------

enum class Role
{
  Admin,
  User,
  Guest
};

static constexpr EnumEntry<Role> roles[] = {
    {"admin", Role::Admin},
    {"user", Role::User},
    {"guest", Role::Guest},
};

static const char *role_name(Role r)
{
  switch (r)
  {
  case Role::Admin:
    return "Admin";
  case Role::User:
    return "User";
  case Role::Guest:
    return "Guest";
  default:
    return "Unknown";
  }
}

// ---------------------------------------------------------------------------
// Demo blocks
// ---------------------------------------------------------------------------

static void demo_int()
{
  std::cout << "\n== to_int<T> (integers)\n";
  for (std::string_view s : {"42", "  -7  ", "999999999999999999999", "12x", ""})
  {
    auto r = to_int<int>(s);

    std::cout << "input='" << s << "' -> ";
    if (!r)
    {
      print_err("to_int<int>", r.error());
      continue;
    }

    std::cout << "value=" << r.value() << "\n";
  }
}

static void demo_float()
{
  std::cout << "\n== to_float<T> (floats)\n";
  for (std::string_view s : {"3.14", "  1e-3 ", "abc", "1.2.3", ""})
  {
    auto r = to_float<double>(s);

    std::cout << "input='" << s << "' -> ";
    if (!r)
    {
      print_err("to_float<double>", r.error());
      continue;
    }

    std::cout << "value=" << r.value() << "\n";
  }
}

static void demo_bool()
{
  std::cout << "\n== to_bool (booleans)\n";
  for (std::string_view s : {"true", "FALSE", "1", "0", "yes", "off", "maybe", ""})
  {
    auto r = to_bool(s);

    std::cout << "input='" << s << "' -> ";
    if (!r)
    {
      print_err("to_bool", r.error());
      continue;
    }

    std::cout << "value=" << (r.value() ? "true" : "false") << "\n";
  }
}

static void demo_enum()
{
  std::cout << "\n== to_enum<T> (enums)\n";
  for (std::string_view s : {"admin", "USER", "guest", "unknown", ""})
  {
    auto r = to_enum<Role>(s, roles); // case-insensitive default=true

    std::cout << "input='" << s << "' -> ";
    if (!r)
    {
      print_err("to_enum<Role>", r.error());
      continue;
    }

    std::cout << "value=" << role_name(r.value()) << "\n";
  }
}

static void demo_parse_generic()
{
  std::cout << "\n== parse<T> (generic facade)\n";

  {
    auto r = parse<int>(" 123 ");
    std::cout << "parse<int>(' 123 ') -> ";
    if (!r)
      print_err("parse<int>", r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = parse<double>(" 3.14 ");
    std::cout << "parse<double>(' 3.14 ') -> ";
    if (!r)
      print_err("parse<double>", r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = parse<bool>("yes");
    std::cout << "parse<bool>('yes') -> ";
    if (!r)
      print_err("parse<bool>", r.error());
    else
      std::cout << (r.value() ? "true" : "false") << "\n";
  }
}

static void demo_to_string()
{
  std::cout << "\n== to_string (formatting)\n";

  {
    auto r = to_string(42);
    std::cout << "to_string(42) -> ";
    if (!r)
      print_err("to_string(int)", r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = to_string(3.14);
    std::cout << "to_string(3.14) -> ";
    if (!r)
      print_err("to_string(float)", r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = to_string(true);
    std::cout << "to_string(true) -> ";
    if (!r)
      print_err("to_string(bool)", r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = to_string(Role::Admin, roles);
    std::cout << "to_string(Role::Admin) -> ";
    if (!r)
      print_err("to_string(enum)", r.error());
    else
      std::cout << r.value() << "\n";
  }
}

// ---------------------------------------------------------------------------
// Bootstrap
// ---------------------------------------------------------------------------

int main()
{
  std::cout << "Vix.cpp - Conversion Showcase\n";

  demo_int();
  demo_float();
  demo_bool();
  demo_enum();
  demo_parse_generic();
  demo_to_string();

  std::cout << "\nDone.\n";
  return 0;
}
