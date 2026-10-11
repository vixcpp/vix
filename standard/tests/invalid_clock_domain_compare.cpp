#include <vix/MonotonicClock.hpp>
#include <vix/WallInstant.hpp>

int main()
{
  const auto wall = vix::WallInstant::from_unix_nanoseconds(0);
  const auto monotonic = vix::MonotonicClock::now();
  return wall < monotonic;
}
