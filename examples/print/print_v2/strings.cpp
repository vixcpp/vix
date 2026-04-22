#include <string>
#include <string_view>

#include <vix/print.hpp>

int main()
{
  std::string name = "Vix";
  std::string_view mode = "stable";

  vix::print("project:", name, "mode:", mode);

  vix::print(
      vix::options{.raw_strings = false},
      "quoted string example");

  return 0;
}
