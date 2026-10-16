#include <vix/Duration.hpp>
#include <vix/files/Files.hpp>
#include <vix/network/Provider.hpp>
#include <vix/process/ProcessDomain.hpp>

int main()
{
  const auto path = vix::files::Path::from_utf8("vix-distribution-sdk-consumer.tmp");
  if (!path || !vix::files::write_utf8(path.value(), "archive-consumer",
                                       vix::files::WriteOptions::create_or_replace()))
    return 1;
  if (!vix::files::remove_file(path.value()))
    return 2;

  const auto executable = vix::process::Executable::search_path("vix");
  if (!executable)
    return 3;
  vix::process::ProcessSpec specification(executable.value());
  if (!specification.add_argument("--version"))
    return 4;

  auto provider = vix::network::Provider::create();
  if (!provider || !provider.value().shutdown())
    return 5;
  return vix::Duration::seconds(1).nanosecond_count() == 1000000000 ? 0 : 6;
}
