#include <iostream>
#include <vix/time/time.hpp>

using namespace vix::time;

int main()
{
  // Date facade
  Date today = Date::now();
  std::cout << "Today (UTC): " << today.to_string() << "\n";

  Date parsed = Date::parse("2026-02-07");
  std::cout << "Parsed date: " << parsed.to_string() << "\n";

  // Convert date to timestamp (00:00:00 UTC)
  Timestamp day_start = parsed.to_timestamp_utc();
  std::cout << "Day start (epoch seconds): "
            << day_start.seconds_since_epoch() << "\n";

  // DateTime
  DateTime now = DateTime::now_utc();
  std::cout << "Now (UTC): " << now.to_string_utc() << "\n";

  DateTime parsed_dt = DateTime::parse("2026-02-07T10:30:15Z");
  std::cout << "Parsed datetime: "
            << parsed_dt.to_string_utc() << "\n";

  // Timestamp + Duration arithmetic
  Timestamp t0 = Timestamp::now();
  Duration wait = Duration::seconds(2);

  Timestamp t1 = t0 + wait;
  Duration delta = t1 - t0;

  std::cout << "Delta (seconds): "
            << delta.count_seconds() << "\n";

  // Steady clock (elapsed time)
  auto start = SteadyClock::now_chrono();
  // simulate work
  int sink = 0;
  for (int i = 0; i < 1'000'000; ++i)
    sink += i;
  Duration elapsed = SteadyClock::since(start);

  std::cout << "Elapsed (ms): "
            << elapsed.count_ms() << "\n";

  return 0;
}
