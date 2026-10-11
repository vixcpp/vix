#include <vix/MonotonicClock.hpp>
#include <vix/WallInstant.hpp>

int main()
{
  const auto wall = vix::WallInstant::from_unix_nanoseconds(0);
  const auto monotonic = vix::MonotonicClock::now();
  [[maybe_unused]] const auto difference = wall - monotonic;
  return 0;
}
