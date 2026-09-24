#include <vix/format.hpp>
#include <vix/print.hpp>

using namespace vix;

int main()
{
  print(format("{0} + {0} = {1}", 2, 4));
  print(format("{2} {1} {0}", "A", "B", "C"));
  return 0;
}
