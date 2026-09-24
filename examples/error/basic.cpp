#include <iostream>
#include <vix/error/Error.hpp>

using namespace vix::error;

int main()
{
  Error err(
      ErrorCode::InvalidArgument,
      ErrorCategory::validation(),
      "port must be greater than 0");

  if (err)
  {
    std::cout << "Error: " << err.message() << "\n";
    std::cout << "Category: " << err.category().name() << "\n";
  }

  return 0;
}
