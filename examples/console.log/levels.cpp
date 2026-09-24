#include <vix/console.hpp>

int main()
{
  vix::console.set_level(vix::Console::Level::Debug);

  vix::console.debug("Debug message visible");
  vix::console.info("Info message visible");
  vix::console.warn("Warning message visible");
  vix::console.error("Error message visible");

  vix::console.set_level(vix::Console::Level::Warn);

  vix::console.debug("This should not appear");
  vix::console.info("This should not appear");
  vix::console.warn("Only warnings and errors now");
  vix::console.error("Still visible");

  return 0;
}
