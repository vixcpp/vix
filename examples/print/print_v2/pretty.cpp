#include <vix/print.hpp>

int main()
{
  std::map<std::string, std::vector<int>> data{
      {"evens", {2, 4, 6, 8}},
      {"odds", {1, 3, 5, 7}},
      {"primes", {2, 3, 5, 7}}};

  vix::print("compact:");
  vix::print(data);

  vix::print();
  vix::print("pretty:");
  vix::print(
      vix::options{
          .compact = false,
          .indent = "    "},
      data);

  return 0;
}
