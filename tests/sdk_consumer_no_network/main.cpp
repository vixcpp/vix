#include <vix/concurrency/ExecutionResource.hpp>
#include <vix/files/Files.hpp>
#include <vix/process/ProcessDomain.hpp>

int main()
{
  auto execution = vix::concurrency::ExecutionResource::create();
  if (!execution)
    return 1;
  auto work = execution.value().submit([] { return 23; });
  if (!work)
    return 2;
  auto completion = work.value().take_completion();
  if (!completion || completion.value().take() != 23)
    return 3;

  auto path = vix::files::Path::from_utf8("vix-sdk-domain-consumer.tmp");
  if (!path)
    return 4;
  if (!vix::files::write_utf8(path.value(), "files-process", vix::files::WriteOptions::create_or_replace()))
    return 5;
  if (!vix::files::remove_file(path.value()))
    return 6;

  auto executable = vix::process::Executable::search_path("vix-sdk-child");
  if (!executable)
    return 7;
  vix::process::ProcessSpec specification(executable.take());
  return specification.add_argument("consumer") ? 0 : 8;
}
