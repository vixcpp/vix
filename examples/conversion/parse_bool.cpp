#include <iostream>
#include <string_view>

#include <vix/conversion/ToBool.hpp>

using namespace vix::conversion;

static void print_err(const ConversionError &e)
{
  std::cout << "error: " << to_string(e.code)
            << " position=" << e.position
            << " input='" << e.input << "'\n";
}

int main()
{
  for (std::string_view s : {"true", "FALSE", "1", "0", "yes", "off", "maybe"})
  {
    auto r = to_bool(s);

    std::cout << "input='" << s << "' -> ";
    if (!r)
    {
      print_err(r.error());
      continue;
    }

    std::cout << "value=" << (r.value() ? "true" : "false") << "\n";
  }

  return 0;
}
