#include <vix/print.hpp>

int main()
{
  vix::print(
      vix::options{.sep = " | "},
      "red", "green", "blue");

  vix::print(
      vix::options{.sep = " -> "},
      "A", "B", "C");

  return 0;
}
