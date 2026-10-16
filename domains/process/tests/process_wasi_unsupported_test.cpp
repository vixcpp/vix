#include <vix/process/ProcessDomain.hpp>

#include <cassert>

int main()
{
  auto path=vix::files::Path::from_utf8("/bin/true");
  assert(path);
  auto executable=vix::process::Executable::path(path.take());
  assert(executable);
  vix::process::ProcessSpec spec(executable.take());
  auto result=vix::process::spawn(std::move(spec));
  assert(!result);
  assert(result.error().identity()==vix::process::errors::unsupported_capability);
}
