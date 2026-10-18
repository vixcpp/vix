#include <vix/concurrency/ExecutionResource.hpp>

#include <atomic>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <mutex>
#include <limits>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using vix::CompletionStatus;
using vix::concurrency::ExecutionResource;
using vix::concurrency::WorkContext;

static_assert(!std::is_copy_constructible_v<ExecutionResource>);
static_assert(std::is_move_constructible_v<ExecutionResource>);
static_assert(std::is_copy_constructible_v<vix::concurrency::WorkHandle<int>>);
static_assert(std::is_nothrow_move_constructible_v<vix::concurrency::WorkHandle<int>>);
static_assert(!std::is_copy_constructible_v<vix::concurrency::NotificationRegistration>);
static_assert(std::is_nothrow_move_constructible_v<vix::concurrency::NotificationRegistration>);
static_assert(!std::is_copy_constructible_v<vix::concurrency::ReadySubscription>);
static_assert(std::is_nothrow_move_constructible_v<vix::concurrency::ReadySubscription>);
static_assert(std::is_same_v<decltype(std::declval<ExecutionResource &>().submit([] { return 1; })),
                             vix::Result<vix::concurrency::WorkHandle<int>>>);
static_assert(std::is_same_v<decltype(std::declval<ExecutionResource &>().submit(
                                 vix::concurrency::WorkOptions{}, [] { return 1; })),
                             vix::Result<vix::concurrency::WorkHandle<int>>>);

namespace
{
  std::atomic<const char *> active_scenario{"startup"};
  std::atomic<const char *> active_phase{"test initialization"};
  std::atomic<const char *> active_worker_phase{"no worker phase recorded"};

  void set_phase(const char *phase) noexcept
  {
    active_phase.store(phase, std::memory_order_relaxed);
  }

  void set_scenario(const char *scenario) noexcept
  {
    active_scenario.store(scenario, std::memory_order_relaxed);
    active_phase.store(scenario, std::memory_order_relaxed);
    active_worker_phase.store("no worker phase recorded", std::memory_order_relaxed);
  }

  void diagnose_termination() noexcept
  {
    std::fprintf(stderr, "Concurrency test terminated in scenario: %s\n",
                 active_scenario.load(std::memory_order_relaxed));
    std::abort();
  }

  struct MoveProbe
  {
    explicit MoveProbe(unsigned int &move_count) noexcept : moves(&move_count) {}
    MoveProbe(const MoveProbe &) = delete;
    MoveProbe &operator=(const MoveProbe &) = delete;
    MoveProbe(MoveProbe &&other) noexcept : moves(other.moves)
    {
      ++*moves;
    }
    unsigned int *moves;
    int operator()() const noexcept { return 8; }
  };

  struct WorkGate
  {
    void enter_and_wait()
    {
      std::unique_lock lock(mutex);
      started = true;
      changed.notify_all();
      changed.wait(lock, [this] { return released; });
    }

    void wait_until_started()
    {
      std::unique_lock lock(mutex);
      changed.wait(lock, [this] { return started; });
    }

    void release()
    {
      {
        std::lock_guard lock(mutex);
        released = true;
      }
      changed.notify_all();
    }

    std::mutex mutex;
    std::condition_variable changed;
    bool started{false};
    bool released{false};
  };

  void require(bool condition,
               const std::source_location location = std::source_location::current())
  {
    if (!condition)
    {
      std::fprintf(stderr, "Concurrency test assertion failed at %s:%u (%s)\n",
                   location.file_name(), static_cast<unsigned int>(location.line()),
                   location.function_name());
      std::terminate();
    }
  }

  void configuration_and_observation()
  {
    auto invalid = ExecutionResource::create({0, 0});
    require(!invalid && invalid.error().identity() == vix::concurrency::errors::invalid_configuration);
    auto overflowing = ExecutionResource::create(
        {1, std::numeric_limits<std::size_t>::max()});
    require(!overflowing && overflowing.error().identity() ==
                                vix::concurrency::errors::invalid_configuration);

    auto created = ExecutionResource::create({2, 8});
    require(static_cast<bool>(created));
    auto resource = created.take();

    auto integer = resource.submit([] { return 42; });
    require(static_cast<bool>(integer));
    auto first_handle = integer.take();
    auto second_handle = first_handle;
    require(static_cast<bool>(first_handle.wait()));
    auto first_result = first_handle.take_completion();
    require(static_cast<bool>(first_result));
    require(first_result.value().status() == CompletionStatus::Success);
    require(first_result.value().take() == 42);
    auto second_result = second_handle.try_take_completion();
    require(!second_result &&
            second_result.error().identity() == vix::concurrency::errors::completion_already_taken);

    auto normalized = resource.submit([] { return vix::Result<int>(19); });
    require(static_cast<bool>(normalized));
    auto normalized_result = normalized.value().take_completion();
    require(static_cast<bool>(normalized_result));
    require(normalized_result.value().take() == 19);
    auto returned_completion = resource.submit([]
    {
      return vix::Completion<int>::cancelled();
    });
    require(static_cast<bool>(returned_completion));
    auto returned_result = returned_completion.value().take_completion();
    require(static_cast<bool>(returned_result));
    require(returned_result.value().status() == CompletionStatus::Cancelled);
    auto returned_failure = resource.submit([]
    {
      return vix::Result<int>(vix::Error(vix::concurrency::errors::resource_creation_failed));
    });
    require(static_cast<bool>(returned_failure));
    auto failure_result = returned_failure.value().take_completion();
    require(static_cast<bool>(failure_result));
    require(failure_result.value().status() == CompletionStatus::Failure);
    require(failure_result.value().error().identity() ==
            vix::concurrency::errors::resource_creation_failed);

    auto void_work = resource.submit([](WorkContext &) {});
    require(static_cast<bool>(void_work));
    auto void_result = void_work.value().take_completion();
    require(static_cast<bool>(void_result));
    require(void_result.value().status() == CompletionStatus::Success);
    auto result_void_work = resource.submit([]
    {
      return vix::Result<void>::success();
    });
    require(static_cast<bool>(result_void_work));
    auto result_void = result_void_work.value().take_completion();
    require(static_cast<bool>(result_void));
    require(result_void.value().status() == CompletionStatus::Success);

    auto raced_observation = resource.submit([] { return 11; });
    require(static_cast<bool>(raced_observation));
    auto observer_a = raced_observation.value();
    auto observer_b = observer_a;
    require(static_cast<bool>(observer_a.wait()));
    std::atomic<unsigned int> observations{0};
    std::atomic<unsigned int> already_taken{0};
    auto observe = [&observations, &already_taken](auto handle)
    {
      auto result = handle.try_take_completion();
      if (result && result.value())
        observations.fetch_add(1, std::memory_order_relaxed);
      else if (!result && result.error().identity() ==
                              vix::concurrency::errors::completion_already_taken)
        already_taken.fetch_add(1, std::memory_order_relaxed);
      else
        std::terminate();
    };
    std::thread observer_thread_a(observe, observer_a);
    std::thread observer_thread_b(observe, observer_b);
    observer_thread_a.join();
    observer_thread_b.join();
    require(observations.load(std::memory_order_relaxed) == 1);
    require(already_taken.load(std::memory_order_relaxed) == 1);

    std::atomic<bool> dropped_handle_work_ran{false};
    {
      auto dropped_handle = resource.submit([&dropped_handle_work_ran]
      {
        dropped_handle_work_ran.store(true, std::memory_order_release);
      });
      require(static_cast<bool>(dropped_handle));
    }
    while (!dropped_handle_work_ran.load(std::memory_order_acquire))
      std::this_thread::yield();
    require(static_cast<bool>(resource.shutdown()));

    auto late_work = resource.submit([] {});
    require(!late_work && late_work.error().identity() ==
                               vix::concurrency::errors::resource_shutting_down);
  }

  void bounded_capacity_and_concurrent_work()
  {
    auto created = ExecutionResource::create({2, 3});
    require(static_cast<bool>(created));
    auto resource = created.take();
    std::mutex mutex;
    std::condition_variable changed;
    bool release = false;
    std::atomic<unsigned int> entered{0};
    std::vector<vix::concurrency::WorkHandle<unsigned int>> work;

    for (unsigned int index = 0; index < 5; ++index)
    {
      auto submitted = resource.submit([&entered, &mutex, &changed, &release, index]
      {
        entered.fetch_add(1, std::memory_order_release);
        changed.notify_all();
        std::unique_lock lock(mutex);
        changed.wait(lock, [&release] { return release; });
        return index;
      });
      require(static_cast<bool>(submitted));
      work.push_back(submitted.take());
    }
    {
      std::unique_lock lock(mutex);
      changed.wait(lock, [&entered] { return entered.load(std::memory_order_acquire) == 2; });
    }
    auto rejected = resource.submit([] { return 99U; });
    require(!rejected && rejected.error().identity() == vix::concurrency::errors::capacity_exhausted);
    unsigned int rejected_moves = 0;
    MoveProbe move_probe(rejected_moves);
    auto rejected_move = resource.submit(std::move(move_probe));
    require(!rejected_move && rejected_moves == 0);
    {
      std::lock_guard lock(mutex);
      release = true;
    }
    changed.notify_all();
    for (auto &handle : work)
    {
      auto completion = handle.take_completion();
      require(static_cast<bool>(completion));
      require(completion.value().status() == CompletionStatus::Success);
    }
    require(static_cast<bool>(resource.shutdown()));

    auto direct = ExecutionResource::create({1, 0});
    require(static_cast<bool>(direct));
    auto direct_resource = direct.take();
    auto admitted = direct_resource.submit([] { return 5; });
    require(static_cast<bool>(admitted));
    require(static_cast<bool>(admitted.value().wait()));
    require(static_cast<bool>(direct_resource.shutdown()));
  }

  void cancellation_exceptions_and_shutdown()
  {
    auto created = ExecutionResource::create({1, 1});
    require(static_cast<bool>(created));
    auto resource = created.take();
    std::mutex mutex;
    std::condition_variable changed;
    bool release = false;
    auto blocker = resource.submit([&mutex, &changed, &release]
    {
      std::unique_lock lock(mutex);
      changed.wait(lock, [&release] { return release; });
      return 1;
    });
    require(static_cast<bool>(blocker));
    auto queued = resource.submit([] { return 2; });
    require(static_cast<bool>(queued));
    require(queued.value().request_cancel());
    require(queued.value().ready());
    {
      std::lock_guard lock(mutex);
      release = true;
    }
    changed.notify_all();
    auto queued_completion = queued.value().take_completion();
    require(static_cast<bool>(queued_completion));
    require(queued_completion.value().status() == CompletionStatus::Cancelled);

    require(static_cast<bool>(blocker.value().wait()));
    require(static_cast<bool>(resource.shutdown()));

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    auto exception_creation = ExecutionResource::create({1, 0});
    require(static_cast<bool>(exception_creation));
    auto exception_resource = exception_creation.take();
    auto throwing = exception_resource.submit([]() -> int { throw std::runtime_error("worker diagnostic"); });
    require(static_cast<bool>(throwing));
    auto thrown = throwing.value().take_completion();
    require(static_cast<bool>(thrown));
    require(thrown.value().status() == CompletionStatus::Failure);
    require(thrown.value().error().identity() == vix::concurrency::errors::worker_exception);
    require(thrown.value().error().message() == "worker diagnostic");
    require(static_cast<bool>(exception_resource.shutdown()));
#endif

    auto cooperative_creation = ExecutionResource::create({1, 1});
    require(static_cast<bool>(cooperative_creation));
    auto cooperative_resource = cooperative_creation.take();
    std::atomic<bool> cooperative_started{false};
    std::atomic<bool> reentrant_shutdown_rejected{false};
    std::mutex lifecycle_mutex;
    std::condition_variable lifecycle_changed;
    bool callback_observed = false;
    auto cooperative = cooperative_resource.submit(
        [&cooperative_started, &cooperative_resource, &reentrant_shutdown_rejected,
         &lifecycle_mutex, &lifecycle_changed, &callback_observed](WorkContext &context)
    {
      auto registration = context.cancellation_token().on_cancel(
          [&cooperative_resource, &reentrant_shutdown_rejected,
           &lifecycle_mutex, &lifecycle_changed, &callback_observed]
          {
            auto shutdown_result = cooperative_resource.shutdown();
            reentrant_shutdown_rejected.store(
                !shutdown_result && shutdown_result.error().identity() ==
                    vix::concurrency::errors::shutdown_from_cancellation_callback,
                std::memory_order_release);
            {
              std::lock_guard lock(lifecycle_mutex);
              callback_observed = true;
            }
            lifecycle_changed.notify_all();
          });
      (void)registration;
      cooperative_started.store(true, std::memory_order_release);
      lifecycle_changed.notify_all();
      active_worker_phase.store("cooperative callable: awaiting cancellation request",
                                std::memory_order_relaxed);
      while (!context.stop_requested())
        std::this_thread::yield();
      // Keep the registration alive until synchronous callback delivery is
      // observed. A condition-variable predicate makes this handshake
      // deterministic without a polling spin.
      active_worker_phase.store("cooperative callable: awaiting cancellation callback",
                                std::memory_order_relaxed);
      std::unique_lock lock(lifecycle_mutex);
      const bool observed = lifecycle_changed.wait_for(
          lock, std::chrono::seconds(5), [&callback_observed] { return callback_observed; });
      lock.unlock();
      require(observed);
      require(reentrant_shutdown_rejected.load(std::memory_order_acquire));
      return 3;
    });
    require(static_cast<bool>(cooperative));
    set_phase("cancellation scenario: waiting for cooperative worker start");
    {
      std::unique_lock lock(lifecycle_mutex);
      require(lifecycle_changed.wait_for(lock, std::chrono::seconds(5), [&cooperative_started]
      {
        return cooperative_started.load(std::memory_order_acquire);
      }));
    }
    auto pending = cooperative_resource.submit([] { return 4; });
    require(static_cast<bool>(pending));
    set_phase("cancellation scenario: ExecutionResource::shutdown and worker join");
    require(static_cast<bool>(cooperative_resource.shutdown()));
    auto cooperative_result = cooperative.value().take_completion();
    require(static_cast<bool>(cooperative_result));
    // A running callable's actual return value is not rewritten as Cancelled.
    require(cooperative_result.value().status() == CompletionStatus::Success);
    auto pending_result = pending.value().take_completion();
    require(static_cast<bool>(pending_result));
    require(pending_result.value().status() == CompletionStatus::Cancelled);
    require(reentrant_shutdown_rejected.load(std::memory_order_acquire));
    auto after_shutdown = cooperative_resource.submit([] {});
    require(!after_shutdown && after_shutdown.error().identity() ==
                                  vix::concurrency::errors::resource_shutting_down);
  }

  void worker_shutdown_rejection_and_race()
  {
    auto created = ExecutionResource::create({2, 8});
    require(static_cast<bool>(created));
    auto resource = created.take();
    auto worker_shutdown = resource.submit([&resource]
    {
      auto result = resource.shutdown();
      return !result && result.error().identity() == vix::concurrency::errors::shutdown_from_worker;
    });
    require(static_cast<bool>(worker_shutdown));
    auto worker_result = worker_shutdown.value().take_completion();
    require(static_cast<bool>(worker_result));
    require(worker_result.value().take());

    constexpr unsigned int submission_count = 100;
    unsigned int attempted = 0;
    unsigned int accepted = 0;
    unsigned int rejected_capacity = 0;
    unsigned int rejected_shutdown = 0;
    std::vector<vix::concurrency::WorkHandle<void>> accepted_handles;
    accepted_handles.reserve(submission_count);
    std::thread submitter([&resource, submission_count, &attempted, &accepted, &rejected_capacity,
                           &rejected_shutdown, &accepted_handles]
    {
      for (unsigned int index = 0; index < submission_count; ++index)
      {
        ++attempted;
        auto result = resource.submit([] {});
        if (result)
        {
          ++accepted;
          accepted_handles.push_back(result.take());
        }
        else if (result.error().identity() == vix::concurrency::errors::capacity_exhausted)
          ++rejected_capacity;
        else if (result.error().identity() == vix::concurrency::errors::resource_shutting_down)
          ++rejected_shutdown;
        else
          require(false);
      }
    });
    std::thread first_shutdown([&resource] { require(static_cast<bool>(resource.shutdown())); });
    std::thread second_shutdown([&resource] { require(static_cast<bool>(resource.shutdown())); });
    submitter.join();
    first_shutdown.join();
    second_shutdown.join();
    require(attempted == submission_count);
    require(accepted + rejected_capacity + rejected_shutdown == attempted);
    require(accepted_handles.size() == accepted);
    for (const auto &handle : accepted_handles)
    {
      require(handle.ready());
      auto completion = handle.take_completion();
      require(static_cast<bool>(completion));
      require(completion.value().status() == CompletionStatus::Success ||
              completion.value().status() == CompletionStatus::Cancelled);
    }
    require(resource.is_shutdown());
  }

  void destruction_waits_for_accepted_work()
  {
    using namespace std::chrono_literals;
    std::atomic<bool> finished{false};
    std::atomic<bool> started{false};
    vix::concurrency::WorkHandle<void> handle;
    {
      auto created = ExecutionResource::create({1, 0});
      require(static_cast<bool>(created));
      auto resource = created.take();
      auto submitted = resource.submit([&finished, &started](WorkContext &context)
      {
        started.store(true, std::memory_order_release);
        while (!context.stop_requested())
          std::this_thread::yield();
        finished.store(true, std::memory_order_release);
      });
      require(static_cast<bool>(submitted));
      handle = submitted.take();
      const auto start_deadline = std::chrono::steady_clock::now() + 5s;
      while (!started.load(std::memory_order_acquire) &&
             std::chrono::steady_clock::now() < start_deadline)
        std::this_thread::yield();
      require(started.load(std::memory_order_acquire));
    }
    require(finished.load(std::memory_order_acquire));
    require(handle.ready());
    auto completion = handle.take_completion();
    require(static_cast<bool>(completion));
    require(completion.value().status() == CompletionStatus::Success);
  }

  void notification_ready_and_single_slot()
  {
    auto created = ExecutionResource::create({1, 1});
    require(static_cast<bool>(created));
    auto resource = created.take();

    auto completed = resource.submit([] { return 17; });
    require(static_cast<bool>(completed));
    require(static_cast<bool>(completed.value().wait()));
    std::atomic<unsigned int> late_calls{0};
    auto late = completed.value().on_ready([&late_calls]() noexcept
    {
      late_calls.fetch_add(1, std::memory_order_relaxed);
    });
    require(static_cast<bool>(late));
    require(late.value().already_ready());
    require(!late.value().take_registration());
    require(late_calls.load(std::memory_order_relaxed) == 0);

    std::mutex mutex;
    std::condition_variable changed;
    bool release = false;
    bool started = false;
    auto pending = resource.submit([&mutex, &changed, &release, &started]
    {
      std::unique_lock lock(mutex);
      started = true;
      changed.notify_all();
      changed.wait(lock, [&release] { return release; });
      return 23;
    });
    require(static_cast<bool>(pending));
    {
      std::unique_lock lock(mutex);
      changed.wait(lock, [&started] { return started; });
    }

    auto copied_handle = pending.value();
    const auto registering_thread = std::this_thread::get_id();
    std::atomic<unsigned int> callback_calls{0};
    std::atomic<bool> callback_context_valid{false};
    auto subscription = pending.value().on_ready(
        [&, owned = std::make_unique<int>(31)]() mutable noexcept
    {
      callback_context_valid.store(
          std::this_thread::get_id() != registering_thread && pending.value().ready() &&
              !resource.is_shutdown() && *owned == 31,
          std::memory_order_release);
      callback_calls.fetch_add(1, std::memory_order_relaxed);
    });
    require(static_cast<bool>(subscription));
    require(!subscription.value().already_ready());
    auto registration_result = subscription.value().take_registration();
    require(static_cast<bool>(registration_result));
    auto registration = registration_result.take();

    auto duplicate = copied_handle.on_ready([]() noexcept {});
    require(!duplicate && duplicate.error().identity() ==
                              vix::concurrency::errors::notification_already_registered);
    {
      std::lock_guard lock(mutex);
      release = true;
    }
    changed.notify_all();
    require(static_cast<bool>(registration.wait()));
    require(callback_calls.load(std::memory_order_relaxed) == 1);
    require(callback_context_valid.load(std::memory_order_acquire));
    auto completion = pending.value().take_completion();
    require(static_cast<bool>(completion) && completion.value().take() == 23);
    require(static_cast<bool>(resource.shutdown()));
  }

  void notification_completion_race_and_shutdown_subscription()
  {
    auto created = ExecutionResource::create({2, 0});
    require(static_cast<bool>(created));
    auto resource = created.take();
    std::mutex mutex;
    std::condition_variable changed;
    bool started = false;
    bool release_work = false;
    std::atomic<unsigned int> calls{0};
    auto work = resource.submit([&]
    {
      std::unique_lock lock(mutex);
      started = true;
      changed.notify_all();
      changed.wait(lock, [&] { return release_work; });
    });
    require(static_cast<bool>(work));
    {
      std::unique_lock lock(mutex);
      changed.wait(lock, [&] { return started; });
    }

    std::barrier<> gate(3);
    std::mutex registration_mutex;
    std::optional<vix::concurrency::NotificationRegistration> registration;
    std::atomic<bool> ready_observed{false};
    std::thread registrar([&]
    {
      gate.arrive_and_wait();
      auto outcome = work.value().on_ready([&calls]() noexcept
      {
        calls.fetch_add(1, std::memory_order_relaxed);
      });
      require(static_cast<bool>(outcome));
      if (outcome.value().already_ready())
        ready_observed.store(true, std::memory_order_release);
      else
      {
        auto taken = outcome.value().take_registration();
        require(static_cast<bool>(taken));
        std::lock_guard lock(registration_mutex);
        registration.emplace(taken.take());
      }
    });
    std::thread completer([&]
    {
      gate.arrive_and_wait();
      {
        std::lock_guard lock(mutex);
        release_work = true;
      }
      changed.notify_all();
    });
    gate.arrive_and_wait();
    registrar.join();
    completer.join();
    {
      std::lock_guard lock(registration_mutex);
      if (registration)
        require(static_cast<bool>(registration->wait()));
    }
    if (ready_observed.load(std::memory_order_acquire))
      require(calls.load(std::memory_order_relaxed) == 0);
    else
      require(calls.load(std::memory_order_relaxed) == 1);

    // A shutdown cancellation callback holds the shutdown thread at a known
    // point while the already-accepted work item is subscribed after closure.
    std::mutex shutdown_mutex;
    std::condition_variable shutdown_changed;
    bool cancellation_callback_entered = false;
    bool release_cancellation_callback = false;
    std::atomic<bool> running_started{false};
    std::atomic<unsigned int> shutdown_calls{0};
    auto running = resource.submit([&](WorkContext &context)
    {
      auto cancellation_registration = context.cancellation_token().on_cancel([&]
      {
        std::unique_lock lock(shutdown_mutex);
        cancellation_callback_entered = true;
        shutdown_changed.notify_all();
        shutdown_changed.wait(lock, [&] { return release_cancellation_callback; });
      });
      (void)cancellation_registration;
      running_started.store(true, std::memory_order_release);
      while (!context.stop_requested())
        std::this_thread::yield();
    });
    require(static_cast<bool>(running));
    while (!running_started.load(std::memory_order_acquire))
      std::this_thread::yield();
    std::thread shutdown_thread([&]
    {
      require(static_cast<bool>(resource.shutdown()));
    });
    {
      std::unique_lock lock(shutdown_mutex);
      shutdown_changed.wait(lock, [&] { return cancellation_callback_entered; });
    }
    require(resource.is_shutdown());
    auto after_closure = running.value().on_ready([&shutdown_calls]() noexcept
    {
      shutdown_calls.fetch_add(1, std::memory_order_relaxed);
    });
    require(static_cast<bool>(after_closure) && !after_closure.value().already_ready());
    auto after_closure_registration = after_closure.value().take_registration();
    require(static_cast<bool>(after_closure_registration));
    auto shutdown_registration = after_closure_registration.take();
    {
      std::lock_guard lock(shutdown_mutex);
      release_cancellation_callback = true;
    }
    shutdown_changed.notify_all();
    shutdown_thread.join();
    require(static_cast<bool>(shutdown_registration.wait()));
    require(shutdown_calls.load(std::memory_order_relaxed) == 1);
  }

  void notification_cancellation_failures_and_exceptions()
  {
    auto created = ExecutionResource::create({1, 1});
    require(static_cast<bool>(created));
    auto resource = created.take();
    auto blocker = resource.submit([&](WorkContext &context)
    {
      while (!context.stop_requested())
        std::this_thread::yield();
      return 1;
    });
    require(static_cast<bool>(blocker));
    auto pending = resource.submit([] { return 2; });
    require(static_cast<bool>(pending));
    std::atomic<unsigned int> cancelled_calls{0};
    auto subscribed = pending.value().on_ready([&cancelled_calls]() noexcept
    {
      cancelled_calls.fetch_add(1, std::memory_order_relaxed);
    });
    require(static_cast<bool>(subscribed));
    auto cancelled_registration_result = subscribed.value().take_registration();
    require(static_cast<bool>(cancelled_registration_result));
    auto cancelled_registration = cancelled_registration_result.take();
    require(pending.value().request_cancel());
    require(pending.value().ready());
    require(static_cast<bool>(blocker.value().request_cancel()));
    require(static_cast<bool>(cancelled_registration.wait()));
    require(cancelled_calls.load(std::memory_order_relaxed) == 1);
    auto cancelled_completion = pending.value().take_completion();
    require(static_cast<bool>(cancelled_completion) &&
            cancelled_completion.value().status() == CompletionStatus::Cancelled);
    WorkGate error_gate;
    auto error_work = resource.submit([&error_gate] { error_gate.enter_and_wait(); });
    require(static_cast<bool>(error_work));
    error_gate.wait_until_started();
    auto error_subscription = error_work.value().on_ready([]
    {
      return vix::Result<void>(vix::Error(vix::concurrency::errors::resource_creation_failed));
    });
    require(static_cast<bool>(error_subscription));
    auto error_registration_result = error_subscription.value().take_registration();
    require(static_cast<bool>(error_registration_result));
    auto error_registration = error_registration_result.take();
    error_gate.release();
    auto error_wait = error_registration.wait();
    require(!error_wait && error_wait.error().identity() ==
                               vix::concurrency::errors::resource_creation_failed);
    auto unaffected_completion = error_work.value().take_completion();
    require(static_cast<bool>(unaffected_completion) &&
            unaffected_completion.value().status() == CompletionStatus::Success);

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    WorkGate worker_exception_gate;
    auto worker_exception_work = resource.submit(
        [&worker_exception_gate]() -> int
        {
          worker_exception_gate.enter_and_wait();
          throw std::runtime_error("worker notification source");
        });
    require(static_cast<bool>(worker_exception_work));
    worker_exception_gate.wait_until_started();
    std::atomic<unsigned int> after_exception_calls{0};
    auto after_exception_subscription = worker_exception_work.value().on_ready(
        [&after_exception_calls]() noexcept
        {
          after_exception_calls.fetch_add(1, std::memory_order_relaxed);
        });
    require(static_cast<bool>(after_exception_subscription));
    auto after_exception_registration_result =
        after_exception_subscription.value().take_registration();
    require(static_cast<bool>(after_exception_registration_result));
    auto after_exception_registration = after_exception_registration_result.take();
    worker_exception_gate.release();
    require(static_cast<bool>(after_exception_registration.wait()));
    auto worker_exception_completion = worker_exception_work.value().take_completion();
    require(static_cast<bool>(worker_exception_completion));
    require(worker_exception_completion.value().status() == CompletionStatus::Failure);
    require(worker_exception_completion.value().error().identity() ==
            vix::concurrency::errors::worker_exception);
    require(after_exception_calls.load(std::memory_order_relaxed) == 1);

    WorkGate standard_exception_gate;
    auto standard_exception_work = resource.submit(
        [&standard_exception_gate] { standard_exception_gate.enter_and_wait(); });
    require(static_cast<bool>(standard_exception_work));
    standard_exception_gate.wait_until_started();
    auto standard_exception = standard_exception_work.value().on_ready([]
    {
      throw std::runtime_error("notification diagnostic");
    });
    require(static_cast<bool>(standard_exception));
    auto standard_registration_result = standard_exception.value().take_registration();
    require(static_cast<bool>(standard_registration_result));
    auto standard_registration = standard_registration_result.take();
    standard_exception_gate.release();
    auto standard_failure = standard_registration.wait();
    require(!standard_failure && standard_failure.error().identity() ==
                                    vix::concurrency::errors::notification_exception);
    require(standard_failure.error().message() == "notification diagnostic");

    WorkGate nonstandard_exception_gate;
    auto nonstandard_exception_work = resource.submit(
        [&nonstandard_exception_gate] { nonstandard_exception_gate.enter_and_wait(); });
    require(static_cast<bool>(nonstandard_exception_work));
    nonstandard_exception_gate.wait_until_started();
    auto nonstandard_exception = nonstandard_exception_work.value().on_ready([]
    {
      throw 7;
    });
    require(static_cast<bool>(nonstandard_exception));
    auto nonstandard_registration_result = nonstandard_exception.value().take_registration();
    require(static_cast<bool>(nonstandard_registration_result));
    auto nonstandard_registration = nonstandard_registration_result.take();
    nonstandard_exception_gate.release();
    auto nonstandard_failure = nonstandard_registration.wait();
    require(!nonstandard_failure && nonstandard_failure.error().identity() ==
                                        vix::concurrency::errors::notification_exception);
#endif
    require(static_cast<bool>(resource.shutdown()));
  }

  void notification_after_shutdown_cancels_pending_work()
  {
    auto created = ExecutionResource::create({1, 1});
    require(static_cast<bool>(created));
    auto resource = created.take();
    std::atomic<bool> running_started{false};
    auto running = resource.submit([&running_started](WorkContext &context)
    {
      running_started.store(true, std::memory_order_release);
      while (!context.stop_requested())
        std::this_thread::yield();
      return 3;
    });
    require(static_cast<bool>(running));
    while (!running_started.load(std::memory_order_acquire))
      std::this_thread::yield();
    auto pending = resource.submit([] { return 4; });
    require(static_cast<bool>(pending));
    std::atomic<unsigned int> calls{0};
    auto subscription = pending.value().on_ready([&calls]() noexcept
    {
      calls.fetch_add(1, std::memory_order_relaxed);
    });
    require(static_cast<bool>(subscription));
    auto registration_result = subscription.value().take_registration();
    require(static_cast<bool>(registration_result));
    auto registration = registration_result.take();
    require(static_cast<bool>(resource.shutdown()));
    require(static_cast<bool>(registration.wait()));
    require(calls.load(std::memory_order_relaxed) == 1);
    auto pending_completion = pending.value().take_completion();
    require(static_cast<bool>(pending_completion) &&
            pending_completion.value().status() == CompletionStatus::Cancelled);
    auto running_completion = running.value().take_completion();
    require(static_cast<bool>(running_completion) &&
            running_completion.value().status() == CompletionStatus::Success);
  }

  void notification_reset_self_deactivation_and_shutdown_wait()
  {
    auto created = ExecutionResource::create({2, 1});
    require(static_cast<bool>(created));
    auto resource = created.take();
    std::mutex mutex;
    std::condition_variable changed;
    bool release_work = false;
    bool callback_entered = false;
    bool release_callback = false;
    auto work = resource.submit([&]
    {
      std::unique_lock lock(mutex);
      changed.wait(lock, [&] { return release_work; });
    });
    require(static_cast<bool>(work));
    auto subscription = work.value().on_ready([&]
    {
      std::unique_lock lock(mutex);
      callback_entered = true;
      changed.notify_all();
      changed.wait(lock, [&] { return release_callback; });
    });
    require(static_cast<bool>(subscription));
    auto registration_result = subscription.value().take_registration();
    require(static_cast<bool>(registration_result));
    auto registration = registration_result.take();
    {
      std::lock_guard lock(mutex);
      release_work = true;
    }
    changed.notify_all();
    {
      std::unique_lock lock(mutex);
      changed.wait(lock, [&] { return callback_entered; });
    }
    std::atomic<bool> reset_started{false};
    std::optional<vix::Result<void>> reset_result;
    std::thread resetter([&]
    {
      reset_started.store(true, std::memory_order_release);
      reset_result.emplace(registration.reset());
    });
    while (!reset_started.load(std::memory_order_acquire))
      std::this_thread::yield();
    {
      std::lock_guard lock(mutex);
      release_callback = true;
    }
    changed.notify_all();
    resetter.join();
    require(reset_result && static_cast<bool>(*reset_result));
    require(!registration.valid());

    WorkGate destruction_gate;
    auto destruction_work = resource.submit(
        [&destruction_gate] { destruction_gate.enter_and_wait(); });
    require(static_cast<bool>(destruction_work));
    destruction_gate.wait_until_started();
    std::mutex destruction_mutex;
    std::condition_variable destruction_changed;
    bool release_destruction_callback = false;
    bool destruction_callback_entered = false;
    auto destruction_subscription = destruction_work.value().on_ready([&]
    {
      std::unique_lock lock(destruction_mutex);
      destruction_callback_entered = true;
      destruction_changed.notify_all();
      destruction_changed.wait(lock, [&] { return release_destruction_callback; });
    });
    require(static_cast<bool>(destruction_subscription));
    auto destruction_registration_result = destruction_subscription.value().take_registration();
    require(static_cast<bool>(destruction_registration_result));
    auto destruction_registration = destruction_registration_result.take();
    destruction_gate.release();
    {
      std::unique_lock lock(destruction_mutex);
      destruction_changed.wait(lock, [&] { return destruction_callback_entered; });
    }
    std::atomic<bool> registration_destroyed{false};
    std::thread destroyer([registration = std::move(destruction_registration),
                           &registration_destroyed]() mutable
    {
      {
        vix::concurrency::NotificationRegistration owned(std::move(registration));
      }
      registration_destroyed.store(true, std::memory_order_release);
    });
    {
      std::lock_guard lock(destruction_mutex);
      release_destruction_callback = true;
    }
    destruction_changed.notify_all();
    destroyer.join();
    require(registration_destroyed.load(std::memory_order_acquire));

    WorkGate self_gate;
    auto self_work = resource.submit([&self_gate] { self_gate.enter_and_wait(); });
    require(static_cast<bool>(self_work));
    self_gate.wait_until_started();
    std::optional<vix::concurrency::NotificationRegistration> self_registration;
    std::atomic<bool> self_deactivated{false};
    std::atomic<bool> self_reset_rejected{false};
    auto self_subscription = self_work.value().on_ready([&]
    {
      auto self_reset = self_registration->reset();
      self_reset_rejected.store(
          !self_reset && self_reset.error().identity() == vix::concurrency::errors::would_deadlock,
          std::memory_order_release);
      self_registration->deactivate();
      self_deactivated.store(true, std::memory_order_release);
    });
    require(static_cast<bool>(self_subscription));
    auto self_registration_result = self_subscription.value().take_registration();
    require(static_cast<bool>(self_registration_result));
    self_registration.emplace(self_registration_result.take());
    self_gate.release();
    require(static_cast<bool>(self_registration->wait()));
    require(self_deactivated.load(std::memory_order_acquire));
    require(self_reset_rejected.load(std::memory_order_acquire));

    // Keep a callback active while shutdown begins; it must not return until
    // the callback has observed shutdown closure and is released below.
    std::mutex shutdown_mutex;
    std::condition_variable shutdown_changed;
    bool callback_observed_shutdown = false;
    bool release_shutdown_callback = false;
    WorkGate shutdown_gate;
    auto shutdown_work = resource.submit([&shutdown_gate] { shutdown_gate.enter_and_wait(); });
    require(static_cast<bool>(shutdown_work));
    shutdown_gate.wait_until_started();
    auto shutdown_subscription = shutdown_work.value().on_ready([&]
    {
      std::unique_lock lock(shutdown_mutex);
      callback_observed_shutdown = true;
      shutdown_changed.notify_all();
      shutdown_changed.wait(lock, [&] { return release_shutdown_callback; });
    });
    require(static_cast<bool>(shutdown_subscription));
    auto shutdown_registration_result = shutdown_subscription.value().take_registration();
    require(static_cast<bool>(shutdown_registration_result));
    auto shutdown_registration = shutdown_registration_result.take();
    shutdown_gate.release();
    std::atomic<bool> shutdown_returned{false};
    std::thread shutdown_thread([&]
    {
      require(static_cast<bool>(resource.shutdown()));
      shutdown_returned.store(true, std::memory_order_release);
    });
    {
      std::unique_lock lock(shutdown_mutex);
      shutdown_changed.wait(lock, [&] { return callback_observed_shutdown; });
    }
    require(!shutdown_returned.load(std::memory_order_acquire));
    {
      std::lock_guard lock(shutdown_mutex);
      release_shutdown_callback = true;
    }
    shutdown_changed.notify_all();
    shutdown_thread.join();
    require(shutdown_returned.load(std::memory_order_acquire));
    require(static_cast<bool>(shutdown_registration.wait()));
  }

  void notification_capacity_and_queued_deactivation()
  {
    auto created = ExecutionResource::create({2, 0});
    require(static_cast<bool>(created));
    auto resource = created.take();

    std::mutex active_mutex;
    std::condition_variable active_changed;
    bool release_active_callback = false;
    WorkGate first_gate;
    std::atomic<bool> active_callback_entered{false};
    auto first = resource.submit([&first_gate] { first_gate.enter_and_wait(); });
    require(static_cast<bool>(first));
    first_gate.wait_until_started();
    auto first_subscription = first.value().on_ready([&]
    {
      std::unique_lock lock(active_mutex);
      active_callback_entered.store(true, std::memory_order_release);
      active_changed.notify_all();
      active_changed.wait(lock, [&] { return release_active_callback; });
    });
    require(static_cast<bool>(first_subscription));
    auto first_registration_result = first_subscription.value().take_registration();
    require(static_cast<bool>(first_registration_result));
    auto first_registration = first_registration_result.take();
    first_gate.release();
    {
      std::unique_lock lock(active_mutex);
      active_changed.wait(lock, [&]
      {
        return active_callback_entered.load(std::memory_order_acquire);
      });
    }

    // The first work record has left the active-work set, but its running
    // notification still owns a bounded notification-capacity reservation.
    WorkGate second_gate;
    auto second = resource.submit([&second_gate] { second_gate.enter_and_wait(); });
    require(static_cast<bool>(second));
    second_gate.wait_until_started();
    auto second_subscription = second.value().on_ready([]() noexcept {});
    require(static_cast<bool>(second_subscription));
    auto second_registration_result = second_subscription.value().take_registration();
    require(static_cast<bool>(second_registration_result));
    auto second_registration = second_registration_result.take();
    auto queued_work = resource.submit([] {});
    require(static_cast<bool>(queued_work));
    auto capacity_failure = queued_work.value().on_ready([]() noexcept {});
    require(!capacity_failure && capacity_failure.error().identity() ==
                                     vix::concurrency::errors::notification_capacity_exhausted);
    second_gate.release();
    {
      std::lock_guard lock(active_mutex);
      release_active_callback = true;
    }
    active_changed.notify_all();
    require(static_cast<bool>(first_registration.wait()));
    require(static_cast<bool>(second_registration.wait()));
    require(static_cast<bool>(queued_work.value().wait()));
    require(static_cast<bool>(resource.shutdown()));

    auto deactivation_created = ExecutionResource::create({2, 1});
    require(static_cast<bool>(deactivation_created));
    auto deactivation_resource = deactivation_created.take();
    std::mutex deactivation_mutex;
    std::condition_variable deactivation_changed;
    bool release_active_callback_for_deactivation = false;
    std::atomic<bool> active_callback_started{false};
    WorkGate active_work_gate;
    auto active_work = deactivation_resource.submit(
        [&active_work_gate] { active_work_gate.enter_and_wait(); });
    require(static_cast<bool>(active_work));
    active_work_gate.wait_until_started();
    auto active_subscription = active_work.value().on_ready([&]
    {
      std::unique_lock lock(deactivation_mutex);
      active_callback_started.store(true, std::memory_order_release);
      deactivation_changed.notify_all();
      deactivation_changed.wait(lock, [&] { return release_active_callback_for_deactivation; });
    });
    require(static_cast<bool>(active_subscription));
    auto active_registration_result = active_subscription.value().take_registration();
    require(static_cast<bool>(active_registration_result));
    auto active_registration = active_registration_result.take();
    active_work_gate.release();
    {
      std::unique_lock lock(deactivation_mutex);
      deactivation_changed.wait(lock, [&]
      {
        return active_callback_started.load(std::memory_order_acquire);
      });
    }

    WorkGate second_deactivation_gate;
    auto second_deactivation_work = deactivation_resource.submit(
        [&second_deactivation_gate] { second_deactivation_gate.enter_and_wait(); });
    require(static_cast<bool>(second_deactivation_work));
    second_deactivation_gate.wait_until_started();
    auto third = deactivation_resource.submit([] {});
    require(static_cast<bool>(third));
    std::atomic<bool> callback_finished{false};
    auto third_subscription = third.value().on_ready([&callback_finished]() noexcept
    {
      callback_finished.store(true, std::memory_order_release);
    });
    require(static_cast<bool>(third_subscription));
    auto third_registration_result = third_subscription.value().take_registration();
    require(static_cast<bool>(third_registration_result));
    auto third_registration = third_registration_result.take();
    require(static_cast<bool>(third.value().request_cancel()));
    third_registration.deactivate();
    require(static_cast<bool>(third_registration.wait()));
    require(!callback_finished.load(std::memory_order_acquire));
    second_deactivation_gate.release();
    {
      std::lock_guard lock(deactivation_mutex);
      release_active_callback_for_deactivation = true;
    }
    deactivation_changed.notify_all();
    require(static_cast<bool>(active_registration.wait()));
    require(static_cast<bool>(second_deactivation_work.value().wait()));
    require(static_cast<bool>(third.value().wait()));
    require(!callback_finished.load(std::memory_order_acquire));
    require(static_cast<bool>(deactivation_resource.shutdown()));
  }

  void deadline_rejection_and_pending_expiration()
  {
    using namespace std::chrono_literals;
    auto created = ExecutionResource::create({1, 1});
    require(static_cast<bool>(created));
    auto resource = created.take();

    std::atomic<bool> rejected_callable_ran{false};
    vix::concurrency::WorkOptions expired_options;
    expired_options.deadline = vix::Deadline::at(vix::MonotonicClock::now());
    auto expired = resource.submit(expired_options, [&rejected_callable_ran]
    {
      rejected_callable_ran.store(true, std::memory_order_release);
    });
    require(!expired && expired.error().identity() == vix::concurrency::errors::deadline_expired);
    require(!rejected_callable_ran.load(std::memory_order_acquire));

    WorkGate occupied_worker;
    auto blocker = resource.submit([&occupied_worker] { occupied_worker.enter_and_wait(); });
    require(static_cast<bool>(blocker));
    occupied_worker.wait_until_started();

    std::atomic<bool> pending_callable_ran{false};
    vix::concurrency::WorkOptions pending_options;
    pending_options.deadline = vix::Deadline::after(vix::Duration::milliseconds(40));
    auto pending = resource.submit(pending_options, [&pending_callable_ran]
    {
      pending_callable_ran.store(true, std::memory_order_release);
      return 2;
    });
    require(static_cast<bool>(pending));

    std::mutex notification_mutex;
    std::condition_variable notification_changed;
    bool notification_called = false;
    auto subscribed = pending.value().on_ready([&]
    {
      {
        std::lock_guard lock(notification_mutex);
        notification_called = true;
      }
      notification_changed.notify_all();
    });
    require(static_cast<bool>(subscribed));
    auto registration_result = subscribed.value().take_registration();
    require(static_cast<bool>(registration_result));
    auto registration = registration_result.take();

    // Completion must progress on the deadline observer while the only
    // execution worker remains occupied. The notification itself waits for
    // that worker and is checked after the gate is released.
    std::mutex completion_mutex;
    std::condition_variable completion_changed;
    bool completion_observed = false;
    std::thread observer([&]
    {
      const auto waited = pending.value().wait();
      {
        std::lock_guard lock(completion_mutex);
        completion_observed = static_cast<bool>(waited);
      }
      completion_changed.notify_all();
    });
    bool completed_while_worker_occupied = false;
    {
      std::unique_lock lock(completion_mutex);
      completed_while_worker_occupied = completion_changed.wait_for(
          lock, 5s, [&completion_observed] { return completion_observed; });
    }
    require(completed_while_worker_occupied);
    require(!pending_callable_ran.load(std::memory_order_acquire));
    {
      std::lock_guard lock(notification_mutex);
      require(!notification_called);
    }

    occupied_worker.release();
    observer.join();
    require(static_cast<bool>(blocker.value().wait()));
    require(static_cast<bool>(registration.wait()));
    {
      std::lock_guard lock(notification_mutex);
      require(notification_called);
    }
    auto timeout = pending.value().take_completion();
    require(static_cast<bool>(timeout));
    require(timeout.value().status() == CompletionStatus::Failure);
    require(timeout.value().error().identity() == vix::concurrency::errors::deadline_expired);
    auto duplicate_timeout = pending.value().try_take_completion();
    require(!duplicate_timeout && duplicate_timeout.error().identity() ==
                                      vix::concurrency::errors::completion_already_taken);

    // The timed-out queue entry retains admission capacity until a worker
    // retires it. Once retirement and notification dispatch are complete,
    // capacity is reusable.
    auto reused = resource.submit(vix::concurrency::WorkOptions{}, [] { return 9; });
    require(static_cast<bool>(reused));
    require(static_cast<bool>(reused.value().wait()));
    require(static_cast<bool>(resource.shutdown()));
    require(pending.value().ready());
    auto duplicate_after_shutdown = pending.value().take_completion();
    require(!duplicate_after_shutdown && duplicate_after_shutdown.error().identity() ==
                                                vix::concurrency::errors::completion_already_taken);
  }

  void deadline_running_cancellation_and_result_preservation()
  {
    using namespace std::chrono_literals;
    auto created = ExecutionResource::create({1, 0});
    require(static_cast<bool>(created));
    auto resource = created.take();
    std::mutex mutex;
    std::condition_variable changed;
    bool started = false;
    bool cancellation_callback_ran = false;

    vix::concurrency::WorkOptions options;
    options.deadline = vix::Deadline::after(vix::Duration::seconds(1));
    auto work = resource.submit(options, [&](WorkContext &context)
    {
      auto registration = context.cancellation_token().on_cancel([&]
      {
        {
          std::lock_guard lock(mutex);
          cancellation_callback_ran = true;
        }
        changed.notify_all();
      });
      (void)registration;
      {
        std::lock_guard lock(mutex);
        started = true;
      }
      changed.notify_all();
      std::unique_lock lock(mutex);
      const bool cancelled = changed.wait_for(lock, 5s, [&]
      {
        return cancellation_callback_ran;
      });
      lock.unlock();
      require(cancelled);
      require(context.stop_requested());
      return 42;
    });
    require(static_cast<bool>(work));
    {
      std::unique_lock lock(mutex);
      require(changed.wait_for(lock, 5s, [&] { return started; }));
    }
    require(static_cast<bool>(work.value().wait()));
    auto result = work.value().take_completion();
    require(static_cast<bool>(result));
    require(result.value().status() == CompletionStatus::Success);
    require(result.value().take() == 42);
    require(static_cast<bool>(resource.shutdown()));
  }

  void deadline_cancellation_and_shutdown_races()
  {
    using namespace std::chrono_literals;
    {
      auto created = ExecutionResource::create({1, 1});
      require(static_cast<bool>(created));
      auto resource = created.take();
      WorkGate occupied_worker;
      auto blocker = resource.submit([&occupied_worker] { occupied_worker.enter_and_wait(); });
      require(static_cast<bool>(blocker));
      occupied_worker.wait_until_started();

      vix::concurrency::WorkOptions options;
      options.deadline = vix::Deadline::after(vix::Duration::milliseconds(40));
      auto pending = resource.submit(options, [] { return 11; });
      require(static_cast<bool>(pending));
      std::barrier start_cancel(2);
      std::thread canceller([&]
      {
        start_cancel.arrive_and_wait();
        (void)pending.value().request_cancel();
      });
      const auto reached_by = std::chrono::steady_clock::now() + 5s;
      while (!options.deadline->reached() && std::chrono::steady_clock::now() < reached_by)
        std::this_thread::yield();
      require(options.deadline->reached());
      start_cancel.arrive_and_wait();
      canceller.join();
      require(static_cast<bool>(pending.value().wait()));
      auto completion = pending.value().take_completion();
      require(static_cast<bool>(completion));
      require(completion.value().status() == CompletionStatus::Cancelled ||
              (completion.value().status() == CompletionStatus::Failure &&
               completion.value().error().identity() == vix::concurrency::errors::deadline_expired));
      auto duplicate = pending.value().try_take_completion();
      require(!duplicate && duplicate.error().identity() ==
                                  vix::concurrency::errors::completion_already_taken);
      occupied_worker.release();
      require(static_cast<bool>(blocker.value().wait()));
      require(static_cast<bool>(resource.shutdown()));
    }

    {
      auto created = ExecutionResource::create({1, 1});
      require(static_cast<bool>(created));
      auto resource = created.take();
      std::atomic<bool> running_started{false};
      auto running = resource.submit([&running_started](WorkContext &context)
      {
        running_started.store(true, std::memory_order_release);
        while (!context.stop_requested())
          std::this_thread::yield();
      });
      require(static_cast<bool>(running));
      const auto started_by = std::chrono::steady_clock::now() + 5s;
      while (!running_started.load(std::memory_order_acquire) &&
             std::chrono::steady_clock::now() < started_by)
        std::this_thread::yield();
      require(running_started.load(std::memory_order_acquire));

      vix::concurrency::WorkOptions options;
      options.deadline = vix::Deadline::after(vix::Duration::milliseconds(40));
      auto pending = resource.submit(options, [] { return 13; });
      require(static_cast<bool>(pending));
      std::atomic<unsigned int> notifications{0};
      auto subscription = pending.value().on_ready([&notifications]() noexcept
      {
        notifications.fetch_add(1, std::memory_order_relaxed);
      });
      require(static_cast<bool>(subscription));
      auto registration_result = subscription.value().take_registration();
      require(static_cast<bool>(registration_result));
      auto registration = registration_result.take();

      std::barrier start_shutdown(2);
      std::thread shutdown_thread([&]
      {
        start_shutdown.arrive_and_wait();
        require(static_cast<bool>(resource.shutdown()));
      });
      const auto reached_by = std::chrono::steady_clock::now() + 5s;
      while (!options.deadline->reached() && std::chrono::steady_clock::now() < reached_by)
        std::this_thread::yield();
      require(options.deadline->reached());
      start_shutdown.arrive_and_wait();
      shutdown_thread.join();
      require(static_cast<bool>(registration.wait()));
      require(notifications.load(std::memory_order_relaxed) == 1);
      auto completion = pending.value().take_completion();
      require(static_cast<bool>(completion));
      require(completion.value().status() == CompletionStatus::Cancelled ||
              (completion.value().status() == CompletionStatus::Failure &&
               completion.value().error().identity() == vix::concurrency::errors::deadline_expired));
      require(running.value().ready());
      require(static_cast<bool>(running.value().take_completion()));
      auto duplicate = pending.value().take_completion();
      require(!duplicate && duplicate.error().identity() ==
                                vix::concurrency::errors::completion_already_taken);
    }
  }
}

int main()
{
  std::mutex watchdog_mutex;
  std::condition_variable watchdog_changed;
  bool test_finished = false;
  std::thread watchdog([&]
  {
    std::unique_lock lock(watchdog_mutex);
    if (!watchdog_changed.wait_for(lock, std::chrono::seconds(8), [&test_finished]
    {
      return test_finished;
    }))
    {
      std::fprintf(stderr,
                   "Concurrency test watchdog: scenario '%s', main phase '%s', "
                   "worker phase '%s'. "
                   "Collect an all-thread debugger backtrace before interruption.\n",
                   active_scenario.load(std::memory_order_relaxed),
                   active_phase.load(std::memory_order_relaxed),
                   active_worker_phase.load(std::memory_order_relaxed));
      std::fflush(stderr);
    }
  });
  std::set_terminate(diagnose_termination);
  set_scenario("configuration_and_observation");
  configuration_and_observation();
  set_scenario("bounded_capacity_and_concurrent_work");
  bounded_capacity_and_concurrent_work();
  set_scenario("cancellation_exceptions_and_shutdown");
  cancellation_exceptions_and_shutdown();
  set_scenario("worker_shutdown_rejection_and_race");
  worker_shutdown_rejection_and_race();
  set_scenario("destruction_waits_for_accepted_work");
  destruction_waits_for_accepted_work();
  set_scenario("notification_ready_and_single_slot");
  notification_ready_and_single_slot();
  set_scenario("notification_completion_race_and_shutdown_subscription");
  notification_completion_race_and_shutdown_subscription();
  set_scenario("notification_cancellation_failures_and_exceptions");
  notification_cancellation_failures_and_exceptions();
  set_scenario("notification_after_shutdown_cancels_pending_work");
  notification_after_shutdown_cancels_pending_work();
  set_scenario("notification_reset_self_deactivation_and_shutdown_wait");
  notification_reset_self_deactivation_and_shutdown_wait();
  set_scenario("notification_capacity_and_queued_deactivation");
  notification_capacity_and_queued_deactivation();
  set_scenario("deadline_rejection_and_pending_expiration");
  deadline_rejection_and_pending_expiration();
  set_scenario("deadline_running_cancellation_and_result_preservation");
  deadline_running_cancellation_and_result_preservation();
  set_scenario("deadline_cancellation_and_shutdown_races");
  deadline_cancellation_and_shutdown_races();
  {
    std::lock_guard lock(watchdog_mutex);
    test_finished = true;
  }
  watchdog_changed.notify_all();
  watchdog.join();
  return 0;
}
