#include <vix/format.hpp>
#include <vix/print.hpp>

using namespace vix;

int main()
{
  std::string out = "Start:";

  format_append(out, " {}", 1);
  format_append(out, " {}", 2);
  format_append(out, " {}", 3);

  print(out); // Start: 1 2 3

  format_to(out, "Reset {}", 42);

  print(out); // Reset 42

  return 0;
}
