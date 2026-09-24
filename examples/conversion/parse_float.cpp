#include <iostream>
#include <string_view>

#include <vix/conversion/ToFloat.hpp>

using namespace vix::conversion;

static void print_err(const ConversionError &e)
{
  std::cout << "error: " << to_string(e.code)
            << " position=" << e.position
            << " input='" << e.input << "'\n";
}

int main()
{
  for (std::string_view s : {"3.14", "  1e-3 ", "abc", "1.2.3"})
  {
    auto r = to_float<double>(s);

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
