/**
 * @file run_command.cpp
 * @brief Demonstrates starting a command and observing its exit status.
 *
 * Requires "git" to be available through PATH.
 */

#include <vix/process/ProcessDomain.hpp>

#include <iostream>

int main()
{
  using namespace vix::process;

  // Selecting an executable does not start a child process.
  auto executable = Executable::search_path("git");
  if (!executable)
  {
    std::cerr << "Invalid executable: " << executable.error().message() << '\n';
    return 1;
  }

  ProcessSpec spec(executable.take());
  auto argument = spec.add_argument("--version");
  if (!argument)
  {
    std::cerr << "Invalid argument: " << argument.error().message() << '\n';
    return 1;
  }

  // Inherited stdout and stderr make the child's output visible directly.
  spec.set_stdout(StandardOutput::inherit());
  spec.set_stderr(StandardError::inherit());

  auto started = spawn(spec);
  if (!started)
  {
    std::cerr << "Cannot start git: " << started.error().message() << '\n';
    return 1;
  }

  auto child = started.take();
  std::cout << "Started child: " << child.id().display() << '\n';

  auto finished = child.wait();
  if (!finished)
  {
    std::cerr << "Wait failed: " << finished.error().message() << '\n';
    return 1;
  }

  const auto &status = finished.value();
  if (status.exit_code())
    std::cout << "Exit code: " << *status.exit_code() << '\n';
  else
    std::cout << "Child terminated without a normal exit code.\n";

  return status.succeeded() ? 0 : 1;
}
