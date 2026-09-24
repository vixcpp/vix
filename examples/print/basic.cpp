#include <vix/print.hpp>
#include <chrono>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

int main()
{
  using namespace std::chrono;

  vix::print_header("Basic");

  vix::print(42, 3.14, "hello", true, nullptr);
  vix::print(std::string{"std::string"});
  vix::print(std::string_view{"string_view"});
  vix::print(std::optional<int>{42});
  vix::print(std::optional<int>{});
  vix::print(std::variant<int, std::string>{123});
  vix::print(std::variant<int, std::string>{std::string{"variant text"}});
  vix::print(std::make_tuple("x", 1, std::optional<int>{42}));

  vix::print(std::vector<int>{1, 2, 3, 4});
  vix::print(std::map<std::string, int>{{"a", 1}, {"b", 2}});
  vix::print(std::filesystem::path{"./data/config.json"});
  vix::print(150ms, 2s, 3min);

  return 0;
}
