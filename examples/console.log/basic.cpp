#include <vix/console.hpp>

int main()
{
  vix::console.log("Hello from Vix console");
  vix::console.info("Application started");
  vix::console.warn("This is a warning");
  vix::console.error("This is an error");

  return 0;
}
