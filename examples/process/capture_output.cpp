/**
 * @file capture_output.cpp
 * @brief Demonstrates capturing a child process's stdout and stderr.
 *
 * Requires "git" to be available through PATH.
 */

#include <vix/process/ProcessDomain.hpp>

#include <array>
#include <cstddef>
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
  auto argument = spec.add_argument("--version");
  if (!argument)
  {
    std::cerr << "Invalid argument: " << argument.error().message() << '\n';
    return 1;
  }

  // Merge stderr into stdout so the parent has only one pipe to drain.
  spec.set_stdout(StandardOutput::pipe());
  spec.set_stderr(StandardError::merge_with_stdout());

  auto started = spawn(spec);
  if (!started)
  {
    std::cerr << "Cannot start git: " << started.error().message() << '\n';
    return 1;
  }

  auto child = started.take();
  auto output = child.take_stdout();
  if (!output)
  {
    std::cerr << "Cannot take output pipe: " << output.error().message() << '\n';
    return 1;
  }

  auto reader = output.take();
  std::array<std::byte, 4096> buffer{};

  // Drain the output before waiting: a full pipe could otherwise block the child.
  while (true)
  {
    auto read = reader.read_some(buffer);
    if (!read)
    {
      std::cerr << "Pipe read failed: " << read.error().message() << '\n';
      return 1;
    }

    const auto count = read.value();
    if (count == 0)
      break; // EOF for a non-empty buffer.

    // ChildOutput returns bytes, not decoded text. Print them as-is here.
    std::cout.write(reinterpret_cast<const char *>(buffer.data()),
                    static_cast<std::streamsize>(count));
  }

  auto closed = reader.close();
  if (!closed)
  {
    std::cerr << "Pipe close failed: " << closed.error().message() << '\n';
    return 1;
  }

  auto finished = child.wait();
  if (!finished)
  {
    std::cerr << "Wait failed: " << finished.error().message() << '\n';
    return 1;
  }

  if (!finished.value().succeeded())
  {
    std::cerr << "Child did not exit successfully.\n";
    return 1;
  }

  return 0;
}
