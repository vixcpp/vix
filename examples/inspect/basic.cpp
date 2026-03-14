#include <vix/inspect.hpp>

#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

int main()
{
  vix::inspect(42);
  vix::inspect(true);
  vix::inspect('A');
  vix::inspect(std::string{"hello inspect"});

  vix::inspect(std::vector<int>{1, 2, 3, 4, 5});
  vix::inspect(std::make_pair(std::string{"age"}, 25));
  vix::inspect(std::make_tuple(1, 3.14, std::string{"tuple"}));

  vix::inspect(std::optional<int>{42});
  vix::inspect(std::optional<int>{});

  std::variant<int, std::string> state = std::string{"active"};
  vix::inspect(state);

  vix::inspect_line("inline", 123, 4.5, false);
  vix::inspect_value("username", std::string{"gaspard"});

  return 0;
}
