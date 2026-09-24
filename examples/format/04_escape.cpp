#include <vix/format.hpp>
#include <vix/print.hpp>

using namespace vix;

int main()
{
  print(format("{{}}")); // {}
  print(format("{{ status = {} }}", "ok"));

  return 0;
}
