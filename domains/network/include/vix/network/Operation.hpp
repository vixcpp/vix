#pragma once

#include <vix/Cancellation.hpp>
#include <vix/Completion.hpp>
#include <vix/Deadline.hpp>
#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace vix::network
{
  struct OperationOptions
  {
    vix::CancellationToken cancellation{};
    std::optional<vix::Deadline> deadline{};
  };

  namespace detail
  {
    template <class T>
    class OperationState
    {
    public:
      virtual ~OperationState() = default;
      virtual bool begin(std::coroutine_handle<>) noexcept = 0;
      virtual bool commit_suspend() noexcept = 0;
      virtual vix::Completion<T> take_completion() = 0;
      virtual void abandon() noexcept = 0;
      virtual bool incomplete() const noexcept = 0;
      virtual bool scheduled() const noexcept = 0;
    };
    template <class T> class OperationFactory;
  }

  template <class T>
  class Operation
  {
  public:
    Operation(Operation &&) noexcept = default;
    Operation(const Operation &) = delete;
    Operation &operator=(Operation &&) = delete;
    Operation &operator=(const Operation &) = delete;
    struct Awaiter
    {
      std::shared_ptr<detail::OperationState<T>> state;
      std::function<void(std::shared_ptr<detail::OperationState<T>>)> starter;
      bool await_ready() const noexcept { return false; }
      bool await_suspend(std::coroutine_handle<> h)
      {
        state->begin(h);
        starter(state);
        return state->commit_suspend();
      }
      vix::Completion<T> await_resume() { return state->take_completion(); }
      ~Awaiter()
      {
        if (!state) return;
        if (state->incomplete()) std::terminate();
        if (state->scheduled()) state->abandon();
      }
    };
    Awaiter operator co_await() && { return {std::move(state_), std::move(starter_)}; }
    Awaiter operator co_await() & = delete;

  private:
    using Starter = std::function<void(std::shared_ptr<detail::OperationState<T>>) >;
    Operation(std::shared_ptr<detail::OperationState<T>> state, Starter starter) : state_(std::move(state)), starter_(std::move(starter)) {}
    std::shared_ptr<detail::OperationState<T>> state_;
    Starter starter_;
    friend class detail::OperationFactory<T>;
  };
}
