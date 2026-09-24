#include <iostream>
#include <string>

#include <vix/validation/Pipe.hpp>

using namespace vix::validation;

int main()
{
  std::string input = "25"; // essaye "abc", "5", "200"

  auto res = validate_parsed<int>("age", input)
                 .between(18, 120, "age must be between 18 and 120")
                 .result("age must be a number");

  std::cout << "ok = " << res.ok() << "\n";

  if (!res.ok())
  {
    for (const auto &e : res.errors.all())
    {
      std::cout << "- field=" << e.field
                << " code=" << to_string(e.code)
                << " msg=" << e.message << "\n";
    }
  }

  return res.ok() ? 0 : 1;
}
