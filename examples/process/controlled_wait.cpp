
/**
 * @file controlled_wait.cpp
 * @brief Demonstrates cancellable and deadline-bounded process waiting.
 *
 * Requires "git" to be available through PATH.
 *
 * The child runs "git hash-object --stdin" and waits for end-of-input.
 * Keeping its stdin pipe open makes it possible to observe a cancelled
 * wait and a timed-out wait without stopping the child itself.
 */

#include <vix/Duration.hpp>
#include <vix/process/ProcessDomain.hpp>

#include <iostream>

int main()
{
  using namespace vix::process;

  auto executable = Executable::search_path("git");
  if (!executable)
  {
    std::cerr << "Invalid executable: " << executable.error().message() << '\n';
    return 1;
  }

  ProcessSpec spec(executable.take());
  if (!spec.add_argument("hash-object") || !spec.add_argument("--stdin"))
  {
    std::cerr << "Could not configure command arguments.\n";
    return 1;
  }

  // The child waits for EOF. Discard its output for this demonstration.
  spec.set_stdin(StandardInput::pipe());
  spec.set_stdout(StandardOutput::null());

  auto started = spawn(spec);
  if (!started)
  {
    std::cerr << "Spawn failed: " << started.error().message() << '\n';
    return 1;
  }

  auto child = started.take();
  auto input_result = child.take_stdin();
  if (!input_result)
  {
    std::cerr << "Cannot access child stdin: " << input_result.error().message() << '\n';
    return 1;
  }
  auto input = input_result.take();

  // Cancellation stops waiting, not the child process.
  vix::CancellationSource source;
  (void)source.request_cancel();
  auto cancelled = child.wait(WaitOptions{source.token()});
  if (cancelled.status() == vix::CompletionStatus::Cancelled)
    std::cout << "First wait was cancelled.\n";
  else if (cancelled.status() == vix::CompletionStatus::Failure)
  {
    std::cerr << "Wait failed: " << cancelled.error().message() << '\n';
    return 1;
  }
  else
    std::cout << "Child finished before cancellation was observed.\n";

  // A deadline expires as Failure(wait_timed_out), not as Cancelled.
  auto bounded = child.wait(WaitOptions{
      {}, vix::Deadline::after(vix::Duration::milliseconds(100))});

  if (bounded.status() == vix::CompletionStatus::Failure)
  {
    if (bounded.error().identity() != errors::wait_timed_out)
    {
      std::cerr << "Wait failed: " << bounded.error().message() << '\n';
      return 1;
    }
    std::cout << "Second wait reached its deadline.\n";
  }
  else if (bounded.status() == vix::CompletionStatus::Cancelled)
  {
    std::cerr << "Unexpected cancellation of the bounded wait.\n";
    return 1;
  }
  else
    std::cout << "Child finished before the deadline.\n";

  // Closing stdin sends EOF so git can finish normally.
  auto closed = input.close();
  if (!closed)
  {
    std::cerr << "Cannot close child stdin: " << closed.error().message() << '\n';
    return 1;
  }

  // We can still wait after cancellation or timeout.
  auto finished = child.wait();
  if (!finished)
  {
    std::cerr << "Final wait failed: " << finished.error().message() << '\n';
    return 1;
  }

  std::cout << "Child exited; success: "
            << (finished.value().succeeded() ? "yes" : "no") << '\n';
  return finished.value().succeeded() ? 0 : 1;
}
