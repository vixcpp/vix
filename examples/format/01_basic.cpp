#include <vix/format.hpp>
#include <vix/print.hpp>

using namespace vix;

int main()
{
  print(format("Hello, {}", "world"));
  return 0;
}
