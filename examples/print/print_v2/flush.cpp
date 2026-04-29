#include <vix/print.hpp>

int main()
{
  vix::print(
      vix::options{
          .end = "",
          .flush = true},
      "processing...");

  vix::print();
  vix::print("done");
  return 0;
}
