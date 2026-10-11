#include <vix/Duration.hpp>
#include <vix/files/Path.hpp>
#include <vix/network/Provider.hpp>
#include <vix/process/Executable.hpp>

int main()
{
  const auto duration = vix::Duration::seconds(1);
  auto path = vix::files::Path::from_utf8("/bin/true");
  if (!path)
    return 1;
  auto executable = vix::process::Executable::path(path.take());
  if (!executable)
    return 2;
  auto provider = vix::network::Provider::create();
  if (!provider)
    return 3;
  return provider.value().shutdown() && duration.nanosecond_count() == 1000000000 ? 0 : 4;
}
