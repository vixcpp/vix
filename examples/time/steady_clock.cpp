#include <iostream>
#include <vix/time/time.hpp>

using namespace vix::time;

int main()
{
  auto start = SteadyClock::now_chrono();

  int sink = 0;
  for (int i = 0; i < 1'000'000; ++i)
    sink += i;

  Duration elapsed = SteadyClock::since(start);

  std::cout << "Elapsed (ms): "
            << elapsed.count_ms() << "\n";
}
