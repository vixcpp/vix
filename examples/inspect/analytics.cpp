#include <vix/inspect.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct Pixel
{
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a;
};

int main()
{
  std::vector<double> latencies{
      2.3, 2.7, 3.1, 2.9, 4.2, 5.0, 3.8, 2.4, 3.0, 4.7};

  vix::inspect_numeric(latencies, "latencies");

  std::vector<int> ids{10, 20, 30, 40, 50};
  vix::inspect_container(ids);

  std::map<std::string, int> counters{
      {"ok", 120},
      {"warn", 7},
      {"error", 2}};
  vix::inspect_container(counters);

  int value = 0x12345678;
  vix::inspect_bytes(value, "int bytes");

  Pixel px{255, 128, 64, 255};
  vix::inspect_bytes(px, "pixel bytes");

  return 0;
}
