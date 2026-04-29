#include <vix/console.hpp>

int main()
{
  vix::console.log("Try with:");
  vix::console.log("VIX_CONSOLE_LEVEL=debug ./your_app");
  vix::console.log("VIX_COLOR=always ./your_app");
  vix::console.log("NO_COLOR=1 ./your_app");

  vix::console.debug("Debug line");
  vix::console.info("Info line");
  vix::console.warn("Warn line");
  vix::console.error("Error line");

  return 0;
}
