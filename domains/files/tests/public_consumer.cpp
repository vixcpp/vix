#include <vix/files/Files.hpp>

int main()
{
  auto path = vix::files::Path::from_utf8("consumer/\xcf\x80/\xf0\x90\x90\xb7.txt");
  if (!path) return 1;
  auto round_trip = path.value().to_utf8();
  return round_trip && round_trip.value() == "consumer/\xcf\x80/\xf0\x90\x90\xb7.txt" ? 0 : 2;
}
