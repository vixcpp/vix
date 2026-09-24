#include <vix/print.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

int main()
{
  using Variant = std::variant<int, std::string>;

  const std::map<std::string, std::vector<Variant>> data = {
      {"numbers", {1, 2, 3, 4}},
      {"mixed", {std::string{"hello"}, 42, std::string{"world"}}}};

  const auto nested = std::make_tuple(
      std::string{"payload"},
      data,
      std::optional<std::vector<int>>{std::vector<int>{7, 8, 9}});

  auto shared = std::make_shared<std::vector<int>>(std::initializer_list<int>{10, 20, 30});

  vix::print_header("Single-line print");
  vix::print(nested);

  vix::print();
  vix::print_header("Pretty print");
  vix::pprint(nested);
  vix::pprint(shared);
  vix::pprint(data);

  return 0;
}
