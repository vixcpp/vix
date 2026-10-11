#include <vix/Deadline.hpp>
#include <vix/Duration.hpp>

#include <cstdlib>
#include <exception>
#include <limits>
#include <string_view>

namespace
{
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
  if (mode == "duration")
  {
    [[maybe_unused]] const auto overflow =
        vix::Duration::max() + vix::Duration::nanoseconds(1);
  }
  if (mode == "deadline")
  {
    [[maybe_unused]] const auto overflow = vix::Deadline::after(
        vix::MonotonicInstant::max(),
        vix::Duration::nanoseconds(1));
  }

  return 1;
}
