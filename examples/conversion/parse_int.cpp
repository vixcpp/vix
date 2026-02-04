#include <iostream>
#include <string_view>

#include <vix/conversion/ToInt.hpp>

using namespace vix::conversion;

static void print_err(const ConversionError &e)
{
  std::cout << "error: " << to_string(e.code)
            << " position=" << e.position
            << " input='" << e.input << "'\n";
}

int main()
{
  for (std::string_view s : {"42", "  -7  ", "999999999999999999999", "12x"})
  {
    auto r = to_int<int>(s);

    std::cout << "input='" << s << "' -> ";
    if (!r)
    {
      print_err(r.error());
      continue;
    }

    std::cout << "value=" << r.value() << "\n";
  }

  return 0;
}
