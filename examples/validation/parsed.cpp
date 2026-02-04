#include <iostream>
#include <string>

#include <vix/validation/Pipe.hpp>

using namespace vix::validation;

int main()
{
  std::string input = "25"; // try "abc"

  auto res = validate_parsed<int>("age", input)
                 .between(18, 120)
                 .result("age must be a number");

  std::cout << "ok=" << res.ok() << "\n";
  return res.ok() ? 0 : 1;
}
