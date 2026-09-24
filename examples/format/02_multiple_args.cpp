#include <vix/format.hpp>
#include <vix/print.hpp>

using namespace vix;

int main()
{
  print(format("{} + {} = {}", 2, 2, 4));
  print(format("Name: {} Age: {}", "Ada", 28));
  return 0;
}
