#include <iostream>

#include <vix/validation/Validate.hpp>

using namespace vix::validation;

int main()
{
  int age = 17;

  auto res = validate("age", age)
                 .min(18, "must be adult")
                 .max(120)
                 .result();

  std::cout << "ok=" << res.ok() << "\n";
  return res.ok() ? 0 : 1;
}
