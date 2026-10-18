#include <vix/concurrency/ExecutionResource.hpp>

void vix_concurrency_exception_disabled_compile_probe()
{
  auto resource = vix::concurrency::ExecutionResource::create({1, 0});
  if (!resource)
    return;
  vix::concurrency::WorkOptions options;
  options.deadline = vix::Deadline::after(vix::Duration::seconds(30));
  auto accepted = resource.value().submit(options, [] { return 17; });
  if (accepted)
  {
    auto notification = accepted.value().on_ready([]() noexcept {});
    if (notification && !notification.value().already_ready())
    {
      auto registration = notification.value().take_registration();
      if (registration)
        registration.value().deactivate();
    }
    (void)accepted.value().request_cancel();
  }
  (void)resource.value().shutdown();
}
