#include <vix/concurrency/ExecutionResource.hpp>

int main()
{
  auto resource = vix::concurrency::ExecutionResource::create();
  if (resource)
    return 1;
  if (resource.error().identity() != vix::concurrency::errors::execution_unavailable)
    return 2;
  vix::concurrency::NotificationRegistration registration;
  if (registration.valid())
    return 3;
  auto notification_wait = registration.wait();
  if (notification_wait || notification_wait.error().identity() !=
                               vix::concurrency::errors::invalid_notification_registration)
    return 4;
  return 0;
}
