#include <vix/Cancellation.hpp>
#include <vix/Completion.hpp>

int main()
{
  vix::CancellationSource source;
  const auto token = source.token();
  int callback_count = 0;
  auto registration = token.on_cancel([&]
                                      { ++callback_count; });
  if (token.is_cancel_requested())
  {
    return 1;
  }

  (void)source.request_cancel();
  return callback_count == 1 && vix::Completion<void>::cancelled().status() ==
                 vix::CompletionStatus::Cancelled
             ? 0
             : 1;
}
