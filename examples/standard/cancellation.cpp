/**
 * @file cancellation.cpp
 * @brief Demonstrates cooperative cancellation and cancellation callbacks.
 *
 * Cancellation requests do not forcibly stop a thread or task.
 * Code receiving a CancellationToken decides when to stop.
 */

#include <vix/Cancellation.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main()
{
  vix::CancellationSource source;
  const auto token = source.token();

  std::atomic<int> steps_completed{0};
  std::atomic<bool> callback_called{false};

  // The registration keeps this callback active until reset or destruction.
  // A cancellation callback runs synchronously on the requesting thread.
  auto registration = token.on_cancel([&]{
     callback_called.store(true);
  });

  std::thread worker([token, &steps_completed]{
    for (int step = 0; step < 100; ++step)
    {
      // The worker cooperatively observes the cancellation request.
      if (token.is_cancel_requested())
        return;

      ++steps_completed;
      std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(120));

  const bool first_request = source.request_cancel();
  const bool second_request = source.request_cancel();

  worker.join();
  registration.reset();

  std::cout << "First request accepted: " << std::boolalpha
            << first_request << '\n';
  std::cout << "Second request accepted: " << second_request << '\n';
  std::cout << "Callback called: " << callback_called.load() << '\n';
  std::cout << "Steps completed: " << steps_completed.load() << '\n';
  std::cout << "Cancellation observed: " << token.is_cancel_requested() << '\n';

  return 0;
}
