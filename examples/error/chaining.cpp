#include <iostream>
#include <string>
#include <vix/error/Result.hpp>

using namespace vix::error;

Result<int> parse_int(const std::string &s)
{
  try
  {
    return std::stoi(s);
  }
  catch (...)
  {
    return Error(
        ErrorCode::ParseError,
        ErrorCategory::validation(),
        "invalid integer");
  }
}

Result<int> validate_port(int port)
{
  if (port <= 0 || port > 65535)
  {
    return Error(
        ErrorCode::InvalidArgument,
        ErrorCategory::validation(),
        "invalid port range");
  }

  return port;
}

int main()
{
  auto result =
      parse_int("8080")
          .and_then(validate_port)
          .map([](int port)
               { return port * 2; });

  if (!result)
  {
    std::cout << "Error: " << result.error().message() << "\n";
    return 0;
  }

  std::cout << "Final value: " << result.value() << "\n";
}
