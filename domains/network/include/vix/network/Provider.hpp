#pragma once

#include <vix/network/Operation.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>
#include <vix/network/Errors.hpp>
#include <vix/Result.hpp>
#include <cstddef>
#include <memory>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>
#include <utility>

namespace vix::network
{
  class Endpoint;
  class TcpConnection;
  class TcpListener;
  class Provider;
  namespace detail { class NetworkAccess; class RunnerAccess; }
  template<class T> vix::Result<vix::Completion<T>> run(Provider &, Operation<T> &&);
  class Provider
  {
  public:
    static vix::Result<Provider> create();
    // Creates a thread-free provider. Operations advance only when poll() is
    // called; the provider and all of its resources are thread-affine to the
    // creating thread. poll() is nonblocking and may not be called recursively.
    static vix::Result<Provider> create_cooperative();
    Provider(Provider &&) noexcept = default;
    Provider(const Provider &) = delete;
    Provider &operator=(Provider &&) = delete;
    Provider &operator=(const Provider &) = delete;
    ~Provider();
    vix::Result<void> shutdown();
    // Performs one nonblocking progress turn: queued commands, expired
    // deadlines, ready native events, then eligible coroutine resumptions.
    // Returns the number of progress actions observed. Valid only for a
    // cooperative provider; a background provider is already self-driven.
    vix::Result<std::size_t> poll();
    Operation<TcpConnection> connect(const Endpoint &, OperationOptions = {});
    vix::Result<TcpListener> listen(const Endpoint &, ListenOptions = {});
  private:
    explicit Provider(std::shared_ptr<void> core) noexcept : core_(std::move(core)) {}
    std::shared_ptr<void> core_;
    friend class detail::NetworkAccess;
    friend class detail::RunnerAccess;
    template<class T> friend vix::Result<vix::Completion<T>> run(Provider &, Operation<T> &&);
  };

  namespace detail
  {
    class RunnerAccess
    {
    public:
      enum class Admission { ready, stopped, reentrant, unsupported };
      static vix::Result<std::size_t> run_one(Provider &);
      static bool cooperative(Provider &) noexcept;
      static Admission admission(Provider &) noexcept;
    };

    template<class T>
    struct RunState
    {
      std::mutex mutex;
      std::condition_variable changed;
      std::optional<vix::Completion<T>> completion;
      bool ready{};

      void publish(vix::Completion<T> value)
      {
        {
          std::lock_guard lock(mutex);
          completion.emplace(std::move(value));
          ready = true;
        }
        changed.notify_all();
      }
    };

    struct DetachedCoroutine
    {
      struct promise_type
      {
        DetachedCoroutine get_return_object() const noexcept { return {}; }
        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }
        void return_void() const noexcept {}
        void unhandled_exception() const noexcept { std::terminate(); }
      };
    };

    template<class T>
    DetachedCoroutine observe_operation(std::shared_ptr<RunState<T>> state,
                                        Operation<T> operation)
    {
      auto completion = co_await std::move(operation);
      state->publish(std::move(completion));
    }
  }

  // Starts one Network operation and synchronously observes its terminal
  // Completion. Cooperative providers are progressed on their owner thread;
  // background providers continue on their existing reactor. This is an
  // entry-point bridge, not a scheduler or a replacement for co_await.
  template<class T>
  vix::Result<vix::Completion<T>> run(Provider &provider, Operation<T> &&operation)
  {
    const auto admission = detail::RunnerAccess::admission(provider);
    if (admission == detail::RunnerAccess::Admission::stopped)
      return vix::Error{provider_stopped, "cannot run an operation on a stopped provider"};
    if (admission == detail::RunnerAccess::Admission::unsupported)
      return vix::Error{unsupported,
                        "blocking cooperative operation runner is unavailable on this platform"};
    if (admission != detail::RunnerAccess::Admission::ready)
      return vix::Error{operation_in_progress,
                        "Network operation runner is unavailable on this provider thread or state"};

    std::shared_ptr<detail::RunState<T>> state;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try
    {
      state = std::make_shared<detail::RunState<T>>();
      (void)detail::observe_operation<T>(state, std::move(operation));
    }
    catch (...)
    {
      return vix::Error{resource_exhausted,
                        "cannot allocate Network operation runner state"};
    }
#else
    state = std::make_shared<detail::RunState<T>>();
    (void)detail::observe_operation<T>(state, std::move(operation));
#endif

    if (detail::RunnerAccess::cooperative(provider))
    {
      for (;;)
      {
        {
          std::lock_guard lock(state->mutex);
          if (state->ready)
            return vix::Result<vix::Completion<T>>{std::move(*state->completion)};
        }
        auto progressed = detail::RunnerAccess::run_one(provider);
        if (!progressed)
        {
          std::unique_lock lock(state->mutex);
          state->changed.wait(lock, [&] { return state->ready; });
          return vix::Result<vix::Completion<T>>{std::move(*state->completion)};
        }
      }
    }

    std::unique_lock lock(state->mutex);
    state->changed.wait(lock, [&] { return state->ready; });
    return vix::Result<vix::Completion<T>>{std::move(*state->completion)};
  }
}
