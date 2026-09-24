#include <vix/print.hpp>

int main()
{
  vix::print(
      vix::options{
          .sep = " :: ",
          .end = " <end>\n",
          .flush = true},
      "alpha", "beta", "gamma");

  return 0;
}
