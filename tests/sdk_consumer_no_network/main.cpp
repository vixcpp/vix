#include <vix/files/Files.hpp>
#include <vix/process/ProcessDomain.hpp>

int main()
{
  auto path = vix::files::Path::from_utf8("vix-sdk-domain-consumer.tmp");
  if (!path)
    return 1;
  if (!vix::files::write_utf8(path.value(), "files-process", vix::files::WriteOptions::create_or_replace()))
    return 2;
  if (!vix::files::remove_file(path.value()))
    return 3;

  auto executable = vix::process::Executable::search_path("vix-sdk-child");
  if (!executable)
    return 4;
  vix::process::ProcessSpec specification(executable.take());
  return specification.add_argument("consumer") ? 0 : 5;
}
