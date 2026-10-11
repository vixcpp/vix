#include <vix/Cancellation.hpp>

#include <cstdlib>
#include <exception>

namespace
{
  [[noreturn]] void expected_termination() noexcept
  {
    std::_Exit(42);
  }
} // namespace

int main()
{
  std::set_terminate(expected_termination);
  vix::CancellationSource source;
  auto registration = source.token().on_cancel([]
                                                { throw 42; });
  (void)registration;
  (void)source.request_cancel();
  return 1;
}
