#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include <vix/print.hpp>

int main()
{
  std::optional<int> age = 25;
  std::variant<int, std::string> status = std::string{"active"};
  auto values = std::make_tuple("tuple", 42, true);
  std::vector<std::string> tags{"fast", "stable", "simple"};
  std::map<std::string, int> stats{{"ok", 10}, {"fail", 2}};

  vix::print(age, status, values, tags, stats);
  return 0;
}
