#include <vix/print.hpp>

int main()
{
  vix::print(
      vix::options{.end = ""},
      "loading...");
  vix::print();

  vix::print(
      vix::options{.end = " <done>\n"},
      "task");

  return 0;
}
