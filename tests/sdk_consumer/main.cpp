#include <vix/Duration.hpp>
#include <vix/concurrency/ExecutionResource.hpp>
#include <vix/files/Files.hpp>
#include <vix/network/Provider.hpp>
#include <vix/process/ProcessDomain.hpp>

#if !defined(__wasi__)
#include <filesystem>
#include <chrono>
#include <string>
#endif

#if !defined(__wasi__)
namespace
{
  std::string utf8(const std::filesystem::path &path)
  {
    const auto bytes = path.u8string();
    return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
  }
}
#endif

int main()
{
  const auto duration = vix::Duration::seconds(1);
#if defined(__wasi__)
  auto execution = vix::concurrency::ExecutionResource::create();
  if (execution || execution.error().identity() != vix::concurrency::errors::execution_unavailable)
    return 1;
  auto path = vix::files::Path::from_utf8("sdk-consumer.txt");
  if (!path)
    return 2;
  auto executable = vix::process::Executable::path(path.take());
  if (!executable)
    return 3;
  vix::process::ProcessSpec process_spec(executable.take());
  if (!process_spec.add_argument("public-sdk"))
    return 4;
  auto provider = vix::network::Provider::create_cooperative();
  if (!provider)
    return 5;
  return provider.value().shutdown() && duration.nanosecond_count() == 1000000000 ? 0 : 6;
#else
  // Two concurrent slots avoid depending on worker retirement timing between
  // observing the first completion and submitting the deadline example.
  auto execution = vix::concurrency::ExecutionResource::create({2, 0});
  if (!execution)
    return 1;
  auto work = execution.value().submit([] { return 7; });
  if (!work)
    return 2;
  auto completion = work.value().take_completion();
  if (!completion || completion.value().take() != 7)
    return 3;
  auto ready_subscription = work.value().on_ready([]() noexcept {});
  if (!ready_subscription)
    return 11;
  if (!ready_subscription.value().already_ready())
  {
    auto registration = ready_subscription.value().take_registration();
    if (!registration)
      return 12;
    auto notification = registration.take();
    if (!notification.wait())
      return 13;
  }
  vix::concurrency::WorkOptions work_options;
  work_options.deadline = vix::Deadline::after(vix::Duration::seconds(5));
  auto deadline_work = execution.value().submit(work_options, [] { return 8; });
  if (!deadline_work)
    return 14;
  auto deadline_completion = deadline_work.value().take_completion();
  if (!deadline_completion || deadline_completion.value().take() != 8)
    return 15;
  const auto file_path = std::filesystem::temp_directory_path() /
    ("vix-sdk-consumer-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt");
  auto path = vix::files::Path::from_utf8(utf8(file_path));
  if (!path)
    return 4;
  if (!vix::files::write_utf8(path.value(), "consumer", vix::files::WriteOptions::create_or_replace()))
    return 5;
  if (!vix::files::remove_file(path.value()))
    return 6;
  auto executable = vix::process::Executable::search_path("vix-sdk-consumer-child");
  if (!executable)
    return 7;
  vix::process::ProcessSpec process_spec(executable.take());
  if (!process_spec.add_argument("public-sdk"))
    return 8;
  auto provider = vix::network::Provider::create();
  if (!provider)
    return 9;
  return provider.value().shutdown() && duration.nanosecond_count() == 1000000000 ? 0 : 10;
#endif
}
