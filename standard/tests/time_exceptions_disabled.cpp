#include <vix/Deadline.hpp>
#include <vix/Duration.hpp>
#include <vix/MonotonicClock.hpp>

int main()
{
  const auto now = vix::MonotonicClock::now();
  const auto deadline = vix::Deadline::after(now, vix::Duration::seconds(5));
  return deadline.reached(now) ? 1 : 0;
}
