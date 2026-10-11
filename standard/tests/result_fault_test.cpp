#include <vix/Result.hpp>

#include <cstdlib>
#include <exception>
#include <string_view>

namespace
{
  inline constexpr vix::ErrorIdentity failed_operation =
      vix::ErrorIdentity::define<"vix.test">(1);

  [[noreturn]] void expected_termination() noexcept
  {
    std::_Exit(42);
  }
} // namespace

int main(int argc, char **argv)
{
  std::set_terminate(expected_termination);

  if (argc != 2)
  {
    return 1;
  }

  const std::string_view mode{argv[1]};

  if (mode == "value")
  {
    vix::Result<int> failure{vix::Error{failed_operation}};
    [[maybe_unused]] const int value = failure.value();
  }

  if (mode == "error")
  {
    vix::Result<int> success{1};
    [[maybe_unused]] const vix::Error &error = success.error();
  }

  if (mode == "taken_value")
  {
    vix::Result<int> success{1};
    [[maybe_unused]] const int taken = success.take();
    [[maybe_unused]] const int value = success.value();
  }

  if (mode == "second_take")
  {
    vix::Result<int> success{1};
    [[maybe_unused]] const int first = success.take();
    [[maybe_unused]] const int second = success.take();
  }

  return 1;
}
