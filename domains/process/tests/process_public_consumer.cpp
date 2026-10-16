#include <vix/process/ProcessDomain.hpp>

int main()
{
  auto executable = vix::process::Executable::search_path("vix-process-consumer");
  if (!executable) return 1;
  vix::process::ProcessSpec specification(executable.take());
  if (!specification.add_argument("public-consumer")) return 2;
  auto environment = vix::process::Environment::empty().with("VIX_CONSUMER", "ok");
  if (!environment) return 3;
  specification.set_environment(environment.take());
  return 0;
}
