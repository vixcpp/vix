#include <iostream>
#include <vix/print.hpp>

int main()
{
  vix::print("normal output");

  vix::print(
      vix::options{.file = &std::cerr},
      "error:", "invalid input");

  return 0;
}
