
/**
 * @file submit_work.cpp
 * @brief Submits bounded concurrent work and observes Vix completions.
 *
 * A WorkHandle is copyable, but its Completion can be taken only once.
 */

#include <vix/concurrency.hpp>

#include <iostream>

int main()
{
  using namespace vix::concurrency;

  // Two workers may execute concurrently; two more tasks may wait in the queue.
  auto created = ExecutionResource::create({2, 2});
  if (!created)
  {
    std::cerr << "Cannot create resource: " << created.error().message() << '\n';
    return 1;
  }
  auto resource = created.take();

  // A task may return an ordinary value ...
  auto first = resource.submit([]
                               { return 6 * 7; });
  if (!first)
  {
    std::cerr << "First submission failed: " << first.error().message() << '\n';
    return 1;
  }

  // ... or a Vix Result<T>. Both become Completion<T> on observation.
  auto second = resource.submit([](WorkContext &context) -> vix::Result<int>
                                {
    if (context.stop_requested())
      return vix::Error{errors::resource_shutting_down, "work was asked to stop"};
    return 10 + 20; });
  if (!second)
  {
    std::cerr << "Second submission failed: " << second.error().message() << '\n';
    return 1;
  }

  auto answer = first.take();
  auto other_observer = answer; // Both handles refer to the same task.
  if (!other_observer.wait())
  {
    std::cerr << "Waiting for the first task failed.\n";
    return 1;
  }

  auto first_result = answer.take_completion();
  if (!first_result || first_result.value().status() != vix::CompletionStatus::Success)
  {
    std::cerr << "The first task did not succeed.\n";
    return 1;
  }
  std::cout << "First result: " << first_result.value().take() << '\n';

  // Copying a handle does not duplicate the result.
  auto taken_again = other_observer.try_take_completion();
  if (taken_again ||
      taken_again.error().identity() != errors::completion_already_taken)
  {
    std::cerr << "Expected the completion to be single-consumer.\n";
    return 1;
  }

  auto second_result = second.value().take_completion();
  if (!second_result || second_result.value().status() != vix::CompletionStatus::Success)
  {
    std::cerr << "The second task did not succeed.\n";
    return 1;
  }
  std::cout << "Second result: " << second_result.value().take() << '\n';

  if (!resource.shutdown())
  {
    std::cerr << "Resource shutdown failed.\n";
    return 1;
  }
  return 0;
}
