#include <vix/process/ProcessDomain.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>

namespace
{
  vix::process::Executable executable(const char *value)
  {
    auto path = vix::files::Path::from_utf8(value);
    assert(path);
    auto result = vix::process::Executable::path(path.take());
    assert(result);
    return result.take();
  }

  vix::process::Process spawn_echo_pipe()
  {
    vix::process::ProcessSpec spec(executable("/bin/echo"));
    assert(spec.add_argument("hello world"));
    spec.set_stdout(vix::process::StandardOutput::pipe());
    auto result = vix::process::spawn(std::move(spec));
    assert(result);
    return result.take();
  }
}

int main()
{
  using namespace vix;
  using namespace vix::process;

  auto process = spawn_echo_pipe();
  auto output = process.take_stdout();
  assert(output);
  std::array<std::byte, 64> bytes{};
  auto read = output.value().read_some(bytes);
  assert(read && read.value() > 0);
  assert(std::string(reinterpret_cast<const char *>(bytes.data()), read.value()) == "hello world\n");
  assert(output.value().close());
  auto exited = process.wait();
  assert(exited && exited.value().succeeded());
  assert(process.wait().value().succeeded());

  ProcessSpec sleeper(executable("/bin/sleep"));
  assert(sleeper.add_argument("1"));
  auto spawned = spawn(std::move(sleeper));
  assert(spawned);
  auto running = spawned.take();
  CancellationSource cancellation;
  assert(cancellation.request_cancel());
  auto cancelled = running.wait(WaitOptions{cancellation.token()});
  assert(cancelled.status() == CompletionStatus::Cancelled);
  assert(running.terminate());
  auto stopped = running.wait();
  assert(stopped && stopped.value().kind() == ExitKind::abnormal);

  ProcessSpec cat_spec(executable("/bin/cat"));
  cat_spec.set_stdin(StandardInput::pipe());
  cat_spec.set_stdout(StandardOutput::pipe());
  auto cat_result = spawn(std::move(cat_spec));
  assert(cat_result);
  auto cat = cat_result.take();
  auto input = cat.take_stdin();
  auto cat_output = cat.take_stdout();
  assert(input && cat_output);
  const std::array<std::byte, 3> payload{std::byte{static_cast<unsigned char>('v')},
                                         std::byte{static_cast<unsigned char>('i')},
                                         std::byte{static_cast<unsigned char>('x')}};
  assert(input.value().write_some(payload).value() == payload.size());
  assert(input.value().close());
  std::array<std::byte, 8> echoed{};
  auto echoed_count = cat_output.value().read_some(echoed);
  assert(echoed_count && echoed_count.value() == payload.size());
  assert(cat.wait().value().succeeded());

  ProcessSpec bad(executable("/bin/echo"));
  auto inherited = Environment::inherit();
  auto custom = inherited.with("VIX_PROCESS_TEST_VALUE", "set");
  assert(custom);
  bad.set_environment(custom.take());
  auto searched = Executable::search_path("echo");
  assert(searched);

  // Dropping Process objects hands only Vix-owned records to the one provider
  // reaper; it neither waits in the destructor nor needs a thread per child.
  for (int i = 0; i < 24; ++i)
  {
    ProcessSpec abandoned(executable("/bin/true"));
    auto child = spawn(std::move(abandoned));
    assert(child);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(40));

  return 0;
}
