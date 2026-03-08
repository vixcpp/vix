#include <vix/print.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

int main()
{
  vix::print_header("Rendering paths");

  vix::print_rendering_paths<
      int,
      bool,
      char,
      std::string,
      std::vector<int>,
      std::map<std::string, int>,
      std::optional<int>,
      std::shared_ptr<int>>();

  vix::print();
  vix::print_header("Checks and diff");

  vix::print_diff(std::vector<int>{1, 2, 3}, std::vector<int>{1, 2, 4}, "left", "right");

  const bool ok1 = vix::print_check("42 == 42", 42, 42);
  const bool ok2 = vix::print_check("3 == 4", 3, 4);

  vix::print_named("ok1", ok1);
  vix::print_named("ok2", ok2);

  vix::print();
  vix::print_header("Typed output / tap");

  vix::print_typed(std::string{"hello"});
  const auto value = vix::tap(std::vector<int>{7, 8, 9}, "captured vector");
  vix::print_named("value again", value);

#ifndef NDEBUG
  vix::print();
  vix::print_header("Debug print");
  vix::dprint("This line appears only in debug builds");
#endif

  return 0;
}
