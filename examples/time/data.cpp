#include <iostream>
#include <vix/time/time.hpp>

using namespace vix::time;

int main()
{
  Date today = Date::now();
  std::cout << "Today (UTC): " << today.to_string() << "\n";

  Date parsed = Date::parse("2026-02-07");
  std::cout << "Parsed date: " << parsed.to_string() << "\n";

  Timestamp day_start = parsed.to_timestamp_utc();
  std::cout << "Day start (epoch seconds): "
            << day_start.seconds_since_epoch() << "\n";
}
