#include <iostream>
#include <string_view>

#include <vix/conversion/Parse.hpp>

using namespace vix::conversion;

static void print_err(const ConversionError &e)
{
  std::cout << "error: " << to_string(e.code)
            << " position=" << e.position
            << " input='" << e.input << "'\n";
}

int main()
{
  {
    auto r = parse<int>(" 123 ");
    std::cout << "parse<int>(' 123 ') -> ";
    if (!r)
      print_err(r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = parse<double>(" 3.14 ");
    std::cout << "parse<double>(' 3.14 ') -> ";
    if (!r)
      print_err(r.error());
    else
      std::cout << r.value() << "\n";
  }

  {
    auto r = parse<bool>("yes");
    std::cout << "parse<bool>('yes') -> ";
    if (!r)
      print_err(r.error());
    else
      std::cout << (r.value() ? "true" : "false") << "\n";
  }

  return 0;
}
