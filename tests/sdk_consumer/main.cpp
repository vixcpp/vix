#include <vix/Duration.hpp>
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
  auto path = vix::files::Path::from_utf8("sdk-consumer.txt");
  if (!path)
    return 1;
  auto executable = vix::process::Executable::path(path.take());
  if (!executable)
    return 2;
  vix::process::ProcessSpec process_spec(executable.take());
  if (!process_spec.add_argument("public-sdk"))
    return 3;
  auto provider = vix::network::Provider::create_cooperative();
  if (!provider)
    return 4;
  return provider.value().shutdown() && duration.nanosecond_count() == 1000000000 ? 0 : 5;
#else
  const auto file_path = std::filesystem::temp_directory_path() /
    ("vix-sdk-consumer-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt");
  auto path = vix::files::Path::from_utf8(utf8(file_path));
  if (!path)
    return 1;
  if (!vix::files::write_utf8(path.value(), "consumer", vix::files::WriteOptions::create_or_replace()))
    return 2;
  if (!vix::files::remove_file(path.value()))
    return 3;
  auto executable = vix::process::Executable::search_path("vix-sdk-consumer-child");
  if (!executable)
    return 4;
  vix::process::ProcessSpec process_spec(executable.take());
  if (!process_spec.add_argument("public-sdk"))
    return 5;
  auto provider = vix::network::Provider::create();
  if (!provider)
    return 6;
  return provider.value().shutdown() && duration.nanosecond_count() == 1000000000 ? 0 : 7;
#endif
}
