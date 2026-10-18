#include <vix/concurrency/ExecutionResource.hpp>

namespace vix::concurrency
{
  Result<ExecutionResource> ExecutionResource::create(Options)
  {
    return Error(errors::execution_unavailable);
  }

  ExecutionResource::ExecutionResource(ExecutionResource &&other) noexcept
      : state_(std::move(other.state_))
  {
  }

  ExecutionResource::~ExecutionResource() = default;

  NotificationRegistration::NotificationRegistration(
      NotificationRegistration &&other) noexcept
      : state_(other.state_.exchange(std::shared_ptr<detail::NotificationState>{},
                                     std::memory_order_acq_rel))
  {
  }

  NotificationRegistration &NotificationRegistration::operator=(
      NotificationRegistration &&other) noexcept
  {
    if (this != &other)
      state_.store(other.state_.exchange(std::shared_ptr<detail::NotificationState>{},
                                         std::memory_order_acq_rel),
                   std::memory_order_release);
    return *this;
  }

  NotificationRegistration::~NotificationRegistration() = default;

  void NotificationRegistration::deactivate() noexcept
  {
  }

  Result<void> NotificationRegistration::reset()
  {
    return Error(errors::invalid_notification_registration);
  }

  Result<void> NotificationRegistration::wait() const
  {
    return Error(errors::invalid_notification_registration);
  }

  Result<NotificationRegistration> ReadySubscription::take_registration()
  {
    return Error(errors::invalid_notification_registration);
  }

  Result<void> ExecutionResource::reserve_work(std::optional<Deadline>)
  {
    return Error(errors::execution_unavailable);
  }

  void detail::notify_deadline_monitor(const std::weak_ptr<detail::ResourceState> &) noexcept
  {
    // Native execution is explicitly unavailable on WASI; no timer thread or
    // fabricated progress mechanism is provided by this provider.
  }

  void ExecutionResource::commit_work(
      const std::shared_ptr<detail::WorkBase> &) noexcept
  {
    std::terminate();
  }

  Result<void> ExecutionResource::shutdown() noexcept
  {
    return Result<void>::success();
  }

  bool ExecutionResource::is_shutdown() const noexcept
  {
    return true;
  }
} // namespace vix::concurrency
