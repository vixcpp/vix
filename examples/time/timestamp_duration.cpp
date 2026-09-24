#include <iostream>
#include <vix/time/time.hpp>

using namespace vix::time;

int main()
{
  Timestamp t0 = Timestamp::now();
  Duration wait = Duration::seconds(2);

  Timestamp t1 = t0 + wait;
  Duration delta = t1 - t0;

  std::cout << "Delta (seconds): "
            << delta.count_seconds() << "\n";
}
