#include <vix/Completion.hpp>
#include <vix/Cancellation.hpp>
#include <vix/Deadline.hpp>

#include <cassert>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
  inline constexpr vix::ErrorIdentity connect_failed =
      vix::ErrorIdentity::define<"vix.network">(1);
  inline constexpr vix::ErrorIdentity deadline_reached =
      vix::ErrorIdentity::define<"vix.network">(2);

  vix::Result<std::unique_ptr<int>> resolve_address(bool fail)
  {
    if (fail)
    {
      return vix::Error{connect_failed, "DNS resolution failed"};
    }
    return std::make_unique<int>(7);
  }

  vix::Completion<std::unique_ptr<int>> connect(
      vix::CancellationToken token,
      bool fail)
  {
    if (token.is_cancel_requested())
    {
      return vix::Completion<std::unique_ptr<int>>::cancelled();
    }

    return resolve_address(fail);
  }

  struct BorrowedSocket
  {
    bool open{true};
  };

  vix::Completion<void> receive_one(
      BorrowedSocket &socket,
      vix::CancellationToken token)
  {
    if (token.is_cancel_requested())
    {
      // Cancellation has no resource-lifecycle authority over this borrow.
      return vix::Completion<void>::cancelled();
    }

    return vix::Completion<void>::success();
  }

  vix::Completion<void> settle_network_operation(
      bool success_became_terminal_first,
      vix::CancellationToken token,
      vix::Deadline deadline,
      vix::MonotonicInstant now)
  {
    if (success_became_terminal_first)
    {
      return vix::Completion<void>::success();
    }
    if (deadline.reached(now))
    {
      // This operation's documented deadline policy is Failure, not Cancelled.
      return vix::Error{deadline_reached, "network operation exceeded its deadline"};
    }
    if (token.is_cancel_requested())
    {
      return vix::Completion<void>::cancelled();
    }
    return vix::Completion<void>::success();
  }

  void test_success_failure_and_cancelled_are_distinct()
  {
    vix::CancellationSource active;
    auto success = connect(active.token(), false);
    assert(success.status() == vix::CompletionStatus::Success);
    assert(*success.take() == 7);
    assert(success.status() == vix::CompletionStatus::Success);

    auto failure = connect(active.token(), true);
    assert(failure.status() == vix::CompletionStatus::Failure);
    assert(failure.error().identity() == connect_failed);

    vix::CancellationSource cancelled;
    assert(cancelled.request_cancel());
    const auto cancelled_completion = connect(cancelled.token(), false);
    assert(cancelled_completion.status() == vix::CompletionStatus::Cancelled);
  }

  void test_result_converts_without_nesting()
  {
    vix::Result<std::string> result{std::string{"connected"}};
    vix::Completion<std::string> completion{std::move(result)};
    assert(completion.status() == vix::CompletionStatus::Success);
    assert(completion.value() == "connected");

    vix::Result<void> result_void = vix::Result<void>::success();
    vix::Completion<void> completion_void{std::move(result_void)};
    assert(completion_void.status() == vix::CompletionStatus::Success);
  }

  void test_result_can_be_an_intentional_success_value()
  {
    // A batch/report value can intentionally contain a Result. This is not an
    // operation terminal model and therefore must remain representable.
    vix::Result<int> per_item{vix::Error{connect_failed, "one item failed"}};
    vix::Completion<vix::Result<int>> batch_report{std::move(per_item)};
    assert(batch_report.status() == vix::CompletionStatus::Success);
    assert(!batch_report.value());
    assert(batch_report.value().error().identity() == connect_failed);
  }

  void test_late_cancellation_cannot_rewrite_terminal_completion()
  {
    vix::CancellationSource source;
    const auto terminal = connect(source.token(), false);
    assert(terminal.status() == vix::CompletionStatus::Success);
    assert(source.request_cancel());
    assert(terminal.status() == vix::CompletionStatus::Success);
  }

  void test_deadline_and_borrowed_resource_policies()
  {
    const auto now = vix::MonotonicClock::now();
    const auto deadline = vix::Deadline::after(now, vix::Duration::seconds(1));
    vix::CancellationSource source;

    const auto timeout = settle_network_operation(
        false,
        source.token(),
        deadline,
        deadline.boundary());
    assert(timeout.status() == vix::CompletionStatus::Failure);
    assert(timeout.error().identity() == deadline_reached);

    assert(source.request_cancel());
    const auto cancelled = settle_network_operation(false, source.token(), deadline, now);
    assert(cancelled.status() == vix::CompletionStatus::Cancelled);

    const auto success_wins = settle_network_operation(
        true,
        source.token(),
        deadline,
        deadline.boundary());
    assert(success_wins.status() == vix::CompletionStatus::Success);

    BorrowedSocket socket;
    const auto receive = receive_one(socket, source.token());
    assert(receive.status() == vix::CompletionStatus::Cancelled);
    assert(socket.open);
  }

  void test_type_properties()
  {
    static_assert(!std::is_default_constructible_v<vix::Completion<int>>);
    static_assert(!std::is_copy_assignable_v<vix::Completion<int>>);
    static_assert(!std::is_move_assignable_v<vix::Completion<int>>);
    static_assert(std::is_move_constructible_v<vix::Completion<std::unique_ptr<int>>>);
    static_assert(!std::is_copy_constructible_v<vix::Completion<std::unique_ptr<int>>>);
  }
} // namespace

int main()
{
  test_success_failure_and_cancelled_are_distinct();
  test_result_converts_without_nesting();
  test_result_can_be_an_intentional_success_value();
  test_late_cancellation_cannot_rewrite_terminal_completion();
  test_deadline_and_borrowed_resource_policies();
  test_type_properties();

  return 0;
}
