#include <vix/console.hpp>

int main()
{
  for (int i = 0; i < 1000; ++i)
  {
    vix::console.log("hot loop message", i);
  }

  vix::console.warn("Warnings are not rate-limited");
  vix::console.error("Errors are not rate-limited");

  return 0;
}
