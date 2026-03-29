#include <vix/format.hpp>
#include <vix/print.hpp>

#include <vector>
#include <map>

using namespace vix;

int main()
{
  print(format("int: {}", 42));
  print(format("float: {}", 3.14));
  print(format("bool: {}", true));

  print(format("vector: {}", std::vector<int>{1, 2, 3}));
  print(format("map: {}", std::map<std::string, int>{{"a", 1}, {"b", 2}}));

  return 0;
}
