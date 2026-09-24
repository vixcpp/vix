#include <iostream>
#include <vix/time/time.hpp>

using namespace vix::time;

int main()
{
  DateTime now = DateTime::now_utc();
  std::cout << "Now (UTC): " << now.to_string_utc() << "\n";

  DateTime parsed = DateTime::parse("2026-02-07T10:30:15Z");
  std::cout << "Parsed datetime: "
            << parsed.to_string_utc() << "\n";
}
