#include <iostream>
#include <vix/error/Result.hpp>

using namespace vix::error;

Result<int> divide(int a, int b)
{
  if (b == 0)
  {
    return Error(
        ErrorCode::InvalidArgument,
        ErrorCategory::validation(),
        "division by zero");
  }

  return a / b;
}

int main()
{
  auto r = divide(10, 0);

  if (!r)
  {
    std::cout << "Error: " << r.error().message() << "\n";
    return 0;
  }

  std::cout << "Result: " << r.value() << "\n";
}
