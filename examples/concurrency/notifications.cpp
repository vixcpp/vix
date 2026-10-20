/**
 * @file notifications.cpp
 * @brief Demonstrates one-shot work completion notifications in Vix.
 *
 * Shows on_ready(), ReadySubscription, NotificationRegistration,
 * waiting for a callback, and deactivating a notification.
 */

#include <vix/concurrency.hpp>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>

namespace
{
  // Keeps a worker busy until the main thread has registered a callback.
  struct Gate
  {
    std::mutex mutex;
    std::condition_variable changed;
    bool started = false;
    bool released = false;

    void block()
    {
      std::unique_lock lock(mutex);
      started = true;
      changed.notify_all();
      changed.wait(lock, [this]
                   { return released; });
    }

    void wait_until_started()
    {
      std::unique_lock lock(mutex);
      changed.wait(lock, [this]
                   { return started; });
    }

    void release()
    {
      {
        std::lock_guard lock(mutex);
        released = true;
      }
      changed.notify_all();
    }
  };
} // namespace

int main()
{
  using namespace vix::concurrency;

  auto created = ExecutionResource::create({1, 1});
  if (!created)
  {
    std::cerr << "Resource creation failed: " << created.error().message() << '\n';
    return 1;
  }
  auto resource = created.take();

  Gate first_gate;
  auto submitted = resource.submit([&first_gate]
                                   {
    first_gate.block();
    return 42; });
  if (!submitted)
  {
    std::cerr << "Work submission failed: " << submitted.error().message() << '\n';
    return 1;
  }
  auto work = submitted.take();
  first_gate.wait_until_started();

  std::atomic<unsigned> notifications{0};

  // on_ready() atomically observes completion or registers one callback.
  // An accepted callback executes once on an ExecutionResource worker.
  auto subscribed = work.on_ready([&notifications]() noexcept
                                  { notifications.fetch_add(1, std::memory_order_relaxed); });
  if (!subscribed)
  {
    first_gate.release();
    std::cerr << "Notification registration failed: "
              << subscribed.error().message() << '\n';
    return 1;
  }

  auto subscription = subscribed.take();
  if (subscription.already_ready())
  {
    first_gate.release();
    std::cerr << "Work unexpectedly completed before registration.\n";
    return 1;
  }

  // Take the registration so its lifetime remains under our control.
  // Leaving it inside ReadySubscription would deactivate it on destruction.
  auto taken = subscription.take_registration();
  if (!taken)
  {
    first_gate.release();
    std::cerr << "Could not take notification registration.\n";
    return 1;
  }
  auto registration = taken.take();

  first_gate.release(); // The task can now complete and trigger its callback.

  // Wait for the callback itself to finish, not just the work item.
  auto notified = registration.wait();
  if (!notified)
  {
    std::cerr << "Notification callback failed: " << notified.error().message() << '\n';
    return 1;
  }

  auto completed = work.take_completion();
  if (!completed || completed.value().status() != vix::CompletionStatus::Success ||
      completed.value().take() != 42 ||
      notifications.load(std::memory_order_relaxed) != 1)
  {
    std::cerr << "Unexpected work or notification result.\n";
    return 1;
  }
  std::cout << "Work completed; notification executed once.\n";

  // A late subscriber observes readiness; its callback is not invoked.
  auto late = work.on_ready([]() noexcept {});
  if (!late || !late.value().already_ready())
  {
    std::cerr << "Expected an already-ready subscription.\n";
    return 1;
  }
  std::cout << "Late subscription: already ready.\n";

  if (!registration.reset())
  {
    std::cerr << "Could not reset the completed registration.\n";
    return 1;
  }

  // Deactivation prevents a not-yet-started callback from running.
  Gate second_gate;
  auto second_submitted = resource.submit([&second_gate]
                                          {
    second_gate.block();
    return 7; });
  if (!second_submitted)
  {
    std::cerr << "Second submission failed.\n";
    return 1;
  }
  auto second_work = second_submitted.take();
  second_gate.wait_until_started();

  std::atomic<unsigned> suppressed{0};
  auto second_subscription = second_work.on_ready([&suppressed]() noexcept
                                                  { suppressed.fetch_add(1, std::memory_order_relaxed); });
  if (!second_subscription)
  {
    second_gate.release();
    std::cerr << "Second notification registration failed.\n";
    return 1;
  }

  auto second_registration_result = second_subscription.value().take_registration();
  if (!second_registration_result)
  {
    second_gate.release();
    std::cerr << "Could not take the second registration.\n";
    return 1;
  }
  auto second_registration = second_registration_result.take();
  second_registration.deactivate(); // Nonblocking; safe before the work finishes.
  second_gate.release();

  auto second_completion = second_work.take_completion();
  auto settled = second_registration.wait();
  if (!second_completion ||
      second_completion.value().status() != vix::CompletionStatus::Success ||
      !settled || suppressed.load(std::memory_order_relaxed) != 0)
  {
    std::cerr << "Deactivated callback ran or work failed.\n";
    return 1;
  }
  std::cout << "Deactivated notification: callback not executed.\n";

  if (!resource.shutdown())
  {
    std::cerr << "Resource shutdown failed.\n";
    return 1;
  }
  return 0;
}
