#include "PrivateChannel.hpp"

#include <vix/network/Errors.hpp>

#include <algorithm>
#include <cstdint>
#include <coroutine>
#include <exception>
#include <new>
#include <utility>

namespace vix::tls::detail
{
  class AsyncGate final : public std::enable_shared_from_this<AsyncGate>
  {
    struct Waiter
    {
      std::coroutine_handle<> continuation{};
      std::weak_ptr<vix::network::detail::ProviderCore> provider;
      std::atomic<bool> cancelled{false};
      std::atomic<bool> timed_out{false};
      std::atomic<bool> granted{false};
      std::atomic<bool> retired{false};
      bool queued{};
      bool committed{};
    };

  public:
    class Guard final
    {
    public:
      explicit Guard(std::shared_ptr<AsyncGate> gate) noexcept : gate_(std::move(gate)) {}
      Guard(const Guard &) = delete;
      Guard &operator=(const Guard &) = delete;
      Guard(Guard &&other) noexcept : gate_(std::move(other.gate_)) {}
      Guard &operator=(Guard &&) = delete;
      ~Guard()
      {
        if (gate_)
          gate_->unlock();
      }

    private:
      std::shared_ptr<AsyncGate> gate_;
    };

    class LockAwaiter final
    {
    public:
      LockAwaiter(std::shared_ptr<AsyncGate> gate,
                  std::shared_ptr<vix::network::detail::ProviderCore> provider,
                  vix::CancellationToken cancellation,
                  std::optional<vix::Deadline> deadline)
          : gate_(std::move(gate)), provider_(std::move(provider)),
            cancellation_(std::move(cancellation)), deadline_(deadline) {}
      LockAwaiter(const LockAwaiter &) = delete;
      LockAwaiter &operator=(const LockAwaiter &) = delete;
      LockAwaiter(LockAwaiter &&) noexcept = default;
      LockAwaiter &operator=(LockAwaiter &&) = delete;

      bool await_ready() const noexcept { return false; }

      bool await_suspend(std::coroutine_handle<> continuation)
      {
        waiter_->continuation = continuation;
        waiter_->provider = provider_;
        registration_ = cancellation_.on_cancel(
            [gate = gate_, waiter = waiter_] { gate->signal(waiter, false); });
        if (deadline_)
        {
          std::weak_ptr<AsyncGate> weak_gate = gate_;
          auto waiter = waiter_;
          deadline_token_ = provider_->add_deadline(
              *deadline_, [weak_gate, waiter]
              {
                if (auto gate = weak_gate.lock())
                  gate->signal(waiter, true);
              });
          if (deadline_token_ == 0)
            waiter_->timed_out.store(true, std::memory_order_release);
        }
        std::lock_guard lock(gate_->mutex_);
        if (waiter_->cancelled.load(std::memory_order_acquire) ||
            waiter_->timed_out.load(std::memory_order_acquire))
          return false;
        if (!gate_->held_)
        {
          gate_->held_ = true;
          waiter_->granted.store(true, std::memory_order_release);
          return false;
        }
        if (gate_->waiter_count_ == gate_->waiters_.size())
          std::terminate(); // At most the read and write parent can contend.
        waiter_->queued = true;
        gate_->waiters_[gate_->waiter_count_++] = waiter_;
        // Publish suspension while holding the same lock used by cancellation
        // and unlock, so a wake cannot be lost between queueing and suspension.
        waiter_->committed = true;
        return true;
      }

      std::optional<Guard> await_resume() noexcept
      {
        registration_.reset();
        waiter_->retired.store(true, std::memory_order_release);
        if (deadline_token_ != 0 &&
            !provider_->post([provider = std::weak_ptr<
                                  vix::network::detail::ProviderCore>{provider_},
                              token = deadline_token_]
                             { if (auto locked = provider.lock())
                                 (void)locked->invalidate_deadline(token); }))
        {
          // The callback retains only gate/waiter state, never the coroutine
          // frame. Provider shutdown discards its remaining timers safely.
        }
        if (!waiter_->granted.load(std::memory_order_acquire))
          return {};
        return Guard{gate_};
      }

    private:
      std::shared_ptr<AsyncGate> gate_;
      std::shared_ptr<vix::network::detail::ProviderCore> provider_;
      vix::CancellationToken cancellation_;
      std::optional<vix::Deadline> deadline_;
      std::uint64_t deadline_token_{};
      std::shared_ptr<Waiter> waiter_{std::make_shared<Waiter>()};
      vix::CancellationRegistration registration_;
    };

    LockAwaiter lock(std::shared_ptr<vix::network::detail::ProviderCore> provider,
                     vix::CancellationToken cancellation,
                     std::optional<vix::Deadline> deadline = {})
    {
      return LockAwaiter{shared_from_this(), std::move(provider),
                         std::move(cancellation), deadline};
    }

  private:
    void signal(const std::shared_ptr<Waiter> &waiter, bool timeout) noexcept
    {
      if (timeout)
        waiter->timed_out.store(true, std::memory_order_release);
      else
        waiter->cancelled.store(true, std::memory_order_release);
      bool resume = false;
      {
        std::lock_guard lock(mutex_);
        if (!waiter->queued)
          return;
        for (std::size_t index = 0; index < waiter_count_; ++index)
        {
          if (waiters_[index] != waiter)
            continue;
          for (std::size_t next = index + 1; next < waiter_count_; ++next)
            waiters_[next - 1] = std::move(waiters_[next]);
          waiters_[--waiter_count_].reset();
          waiter->queued = false;
          resume = waiter->committed &&
                   !waiter->retired.load(std::memory_order_acquire);
          break;
        }
      }
      if (resume)
      {
        if (auto provider = waiter->provider.lock())
        {
          if (!provider->post([continuation = waiter->continuation]
                              { continuation.resume(); }))
            std::terminate(); // An active Network operation pins this provider.
        }
      }
    }

    void unlock() noexcept
    {
      std::shared_ptr<Waiter> waiter;
      {
        std::lock_guard lock(mutex_);
        if (waiter_count_ == 0)
        {
          held_ = false;
          return;
        }
        waiter = std::move(waiters_[0]);
        for (std::size_t index = 1; index < waiter_count_; ++index)
          waiters_[index - 1] = std::move(waiters_[index]);
        waiters_[--waiter_count_].reset();
        waiter->queued = false;
        waiter->granted.store(true, std::memory_order_release);
      }
      waiter->continuation.resume();
    }

    std::mutex mutex_;
    bool held_{};
    std::array<std::shared_ptr<Waiter>, 2> waiters_{};
    std::size_t waiter_count_{};
  };

  struct PrivateChannel::State
  {
    explicit State(std::unique_ptr<OpenSslDriver> owned_driver)
        : driver(std::move(owned_driver)) {}

    ~State()
    {
      if (lease)
        lease->close();
    }

    std::optional<vix::network::detail::TcpTransportLease> lease;
    std::unique_ptr<OpenSslDriver> driver;
    std::shared_ptr<vix::network::detail::ProviderCore> provider;
    std::mutex operation_mutex;
    bool handshake_started{};
    bool handshake_active{};
    bool handshake_complete{};
    bool invalidated{};
    bool peer_close_received{};
    bool read_active{};
    bool write_active{};
    bool shutdown_started{};
    bool shutdown_active{};
    std::atomic<std::size_t> shutdown_ciphertext_generated{};
    std::atomic<std::size_t> shutdown_ciphertext_sent{};
    std::optional<vix::CancellationSource> handshake_cancellation;
    std::optional<vix::CancellationSource> read_cancellation;
    std::optional<vix::CancellationSource> write_cancellation;
    std::optional<vix::CancellationSource> shutdown_cancellation;
    std::shared_ptr<AsyncGate> receive_gate{std::make_shared<AsyncGate>()};
    std::shared_ptr<AsyncGate> send_gate{std::make_shared<AsyncGate>()};
    std::shared_ptr<AsyncGate> protocol_gate{std::make_shared<AsyncGate>()};
    std::mutex inbound_mutex;
    std::array<std::byte, OpenSslDriver::buffer_capacity> ciphertext{};
    std::array<std::byte, OpenSslDriver::buffer_capacity> inbound{};
    std::size_t inbound_size{};
    std::size_t inbound_offset{};
    std::uint64_t input_generation{};
  };

  namespace
  {
    bool wrong_cooperative_thread(
        const std::shared_ptr<PrivateChannel::State> &state) noexcept
    {
      return state->provider && state->provider->cooperative() &&
             !state->provider->on_owner_thread();
    }

    struct ParentFrame
    {
      struct promise_type
      {
        ParentFrame get_return_object() noexcept { return {}; }
        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }
        void return_void() const noexcept {}
        void unhandled_exception() const noexcept { std::terminate(); }
      };
    };

    vix::Completion<void> failure_completion(vix::ErrorIdentity identity,
                                              const char *message)
    {
      return vix::Completion<void>{vix::Error{identity, message}};
    }

    template <class T>
    vix::Completion<T> failure_completion(vix::ErrorIdentity identity,
                                          const char *message)
    {
      return vix::Completion<T>{vix::Error{identity, message}};
    }

    vix::network::OperationOptions child_options(
        vix::CancellationSource &source,
        const std::optional<vix::Deadline> &deadline)
    {
      return {source.token(), deadline};
    }

    void close_lease_on_provider(const std::shared_ptr<PrivateChannel::State> &state) noexcept
    {
      if (!state->lease || !state->provider)
        return;
      if (state->provider->cooperative() && !state->provider->on_owner_thread())
      {
        if (!state->provider->post([state] { if (state->lease) state->lease->close(); }))
          std::terminate(); // The lease keeps provider shutdown from succeeding.
        return;
      }
      if (state->lease)
        state->lease->close();
    }

    void request_channel_close(const std::shared_ptr<PrivateChannel::State> &state) noexcept
    {
      std::optional<vix::CancellationSource> cancel_handshake;
      std::optional<vix::CancellationSource> cancel_read;
      std::optional<vix::CancellationSource> cancel_write;
      std::optional<vix::CancellationSource> cancel_shutdown;
      bool close_transport = false;
      {
        std::lock_guard lock(state->operation_mutex);
        state->invalidated = true;
        if (state->handshake_active)
          cancel_handshake = state->handshake_cancellation;
        if (state->read_active)
          cancel_read = state->read_cancellation;
        if (state->write_active)
          cancel_write = state->write_cancellation;
        if (state->shutdown_active)
          cancel_shutdown = state->shutdown_cancellation;
        close_transport = !state->handshake_active && !state->read_active &&
                          !state->write_active && !state->shutdown_active;
      }
      if (cancel_handshake)
        (void)cancel_handshake->request_cancel();
      if (cancel_read)
        (void)cancel_read->request_cancel();
      if (cancel_write)
        (void)cancel_write->request_cancel();
      if (cancel_shutdown)
        (void)cancel_shutdown->request_cancel();
      if (close_transport)
        close_lease_on_provider(state);
    }

    bool finish_handshake(const std::shared_ptr<PrivateChannel::State> &state,
                          bool invalidate, bool publish_success = false,
                          bool retry_before_engine_entry = false) noexcept
    {
      std::optional<vix::CancellationSource> cancel_read;
      std::optional<vix::CancellationSource> cancel_write;
      bool close_transport = false;
      bool invalidated_now = false;
      {
        std::lock_guard lock(state->operation_mutex);
        state->handshake_active = false;
        state->handshake_cancellation.reset();
        state->invalidated = state->invalidated || invalidate;
        if (retry_before_engine_entry && !state->invalidated)
          state->handshake_started = false;
        if (publish_success && !state->invalidated)
          state->handshake_complete = true;
        invalidated_now = state->invalidated;
        if (state->invalidated)
        {
          if (state->read_active)
            cancel_read = state->read_cancellation;
          if (state->write_active)
            cancel_write = state->write_cancellation;
          close_transport = !state->read_active && !state->write_active;
        }
      }
      if (cancel_read)
        (void)cancel_read->request_cancel();
      if (cancel_write)
        (void)cancel_write->request_cancel();
      if (close_transport)
        close_lease_on_provider(state);
      return invalidated_now;
    }

    void finish_parent(const std::shared_ptr<PrivateChannel::State> &state,
                       bool read, bool invalidate) noexcept
    {
      std::optional<vix::CancellationSource> cancel_handshake;
      std::optional<vix::CancellationSource> cancel_read;
      std::optional<vix::CancellationSource> cancel_write;
      bool close_transport = false;
      {
        std::lock_guard lock(state->operation_mutex);
        if (read)
        {
          state->read_active = false;
          state->read_cancellation.reset();
        }
        else
        {
          state->write_active = false;
          state->write_cancellation.reset();
        }
        state->invalidated = state->invalidated || invalidate;
        if (state->invalidated)
        {
          if (state->handshake_active)
            cancel_handshake = state->handshake_cancellation;
          if (state->read_active)
            cancel_read = state->read_cancellation;
          if (state->write_active)
            cancel_write = state->write_cancellation;
          close_transport = !state->handshake_active && !state->read_active &&
                            !state->write_active;
        }
      }
      // Standard cancellation callbacks are synchronous. Never request them
      // while holding the channel state mutex.
      if (cancel_handshake)
        (void)cancel_handshake->request_cancel();
      if (cancel_read)
        (void)cancel_read->request_cancel();
      if (cancel_write)
        (void)cancel_write->request_cancel();
      if (close_transport)
        close_lease_on_provider(state);
    }

    vix::Result<void> finish_shutdown(
        const std::shared_ptr<PrivateChannel::State> &state) noexcept
    {
      {
        std::lock_guard lock(state->operation_mutex);
        state->shutdown_active = false;
        state->shutdown_cancellation.reset();
        state->invalidated = true;
      }
      auto result = state->lease
          ? state->lease->shutdown_write()
          : vix::Result<void>{vix::Error{channel_closed, "TLS transport is unavailable"}};
      close_lease_on_provider(state);
      return result;
    }

    std::optional<std::size_t> feed_retained_input(
        const std::shared_ptr<PrivateChannel::State> &state) noexcept
    {
      std::lock_guard lock(state->inbound_mutex);
      if (state->inbound_offset == state->inbound_size)
      {
        state->inbound_offset = 0;
        state->inbound_size = 0;
        return std::size_t{0};
      }
      auto fed = state->driver->feed_ciphertext(
          std::span<const std::byte>{state->inbound.data() + state->inbound_offset,
                                     state->inbound_size - state->inbound_offset});
      if (!fed)
        return std::nullopt;
      state->inbound_offset += fed.value();
      if (state->inbound_offset == state->inbound_size)
      {
        state->inbound_offset = 0;
        state->inbound_size = 0;
      }
      return fed.value();
    }

    std::uint64_t received_input_generation(
        const std::shared_ptr<PrivateChannel::State> &state) noexcept
    {
      std::lock_guard lock(state->inbound_mutex);
      return state->input_generation;
    }

    bool retained_input_pending(
        const std::shared_ptr<PrivateChannel::State> &state) noexcept
    {
      std::lock_guard lock(state->inbound_mutex);
      return state->inbound_offset < state->inbound_size;
    }

    bool retain_received_input(const std::shared_ptr<PrivateChannel::State> &state,
                               std::size_t count) noexcept
    {
      std::lock_guard lock(state->inbound_mutex);
      state->inbound_offset = 0;
      state->inbound_size = count;
      ++state->input_generation;
      return true;
    }

    ParentFrame drive_handshake(
        std::shared_ptr<PrivateChannel::State> state,
        std::shared_ptr<vix::network::detail::OperationRecord<void>> parent,
        vix::network::OperationOptions options,
        vix::CancellationSource cancellation)
    {
      bool engine_entered = false;
      try
      {
        auto cancellation_registration = options.cancellation.on_cancel(
            [cancellation] { (void)cancellation.request_cancel(); });
        const auto child_operation_options = child_options(cancellation, options.deadline);
        for (;;)
        {
          if (cancellation.is_cancel_requested())
          {
            (void)finish_handshake(state, engine_entered, false, !engine_entered);
            parent->complete(vix::Completion<void>::cancelled());
            co_return;
          }
          if (options.deadline && options.deadline->reached())
          {
            (void)finish_handshake(state, engine_entered, false, !engine_entered);
            parent->complete(failure_completion(vix::network::timed_out,
                                                "TLS handshake deadline reached"));
            co_return;
          }

          const auto progress = state->driver->handshake_step();
          engine_entered = true;
          bool sent_ciphertext = false;
          while (state->driver->pending_ciphertext() != 0)
          {
            auto drained = state->driver->drain_ciphertext(state->ciphertext);
            if (!drained || drained.value() == 0)
            {
              (void)finish_handshake(state, true);
              parent->complete(failure_completion(backend_failure,
                                                  "TLS outbound BIO could not be drained"));
              co_return;
            }
            const auto count = drained.value();
            auto sent = co_await state->lease->send(
                std::span<const std::byte>{state->ciphertext.data(), count},
                child_operation_options);
            if (sent.status() != vix::CompletionStatus::Success)
            {
              (void)finish_handshake(state, true);
              if (sent.status() == vix::CompletionStatus::Cancelled)
                parent->complete(vix::Completion<void>::cancelled());
              else
                parent->complete(vix::Completion<void>{sent.error()});
              co_return;
            }
            sent_ciphertext = true;
          }

          bool fed_ciphertext = false;
          const bool has_retained_input = state->inbound_offset < state->inbound_size;
          if (has_retained_input)
          {
            auto fed = state->driver->feed_ciphertext(
                std::span<const std::byte>{state->inbound.data() + state->inbound_offset,
                                           state->inbound_size - state->inbound_offset});
            if (!fed)
            {
              (void)finish_handshake(state, true);
              parent->complete(failure_completion(buffer_limit,
                                                  "TLS inbound BIO cannot accept retained ciphertext"));
              co_return;
            }
            state->inbound_offset += fed.value();
            fed_ciphertext = fed.value() != 0;
            if (state->inbound_offset == state->inbound_size)
            {
              state->inbound_offset = 0;
              state->inbound_size = 0;
            }
          }

          if (progress.want == EngineWant::complete)
          {
            const bool invalidated = finish_handshake(state, false, true);
            if (invalidated)
              parent->complete(vix::Completion<void>::cancelled());
            else
              parent->complete(vix::Completion<void>{vix::Result<void>::success()});
            co_return;
          }
          if (progress.want == EngineWant::failure || progress.want == EngineWant::closed)
          {
            (void)finish_handshake(state, true);
            parent->complete(failure_completion(progress.error,
                                                "TLS handshake backend failed"));
            co_return;
          }
          if (sent_ciphertext || fed_ciphertext)
            continue;
          if (progress.want == EngineWant::write)
          {
            (void)finish_handshake(state, true);
            parent->complete(failure_completion(backend_failure,
                                                "OpenSSL requested output with no buffered ciphertext"));
            co_return;
          }
          if (has_retained_input)
          {
            (void)finish_handshake(state, true);
            parent->complete(failure_completion(buffer_limit,
                                                "TLS inbound BIO cannot accept more retained ciphertext"));
            co_return;
          }
          if (progress.want != EngineWant::read)
          {
            (void)finish_handshake(state, true);
            parent->complete(failure_completion(backend_failure,
                                                "TLS driver made no transport progress"));
            co_return;
          }

          auto received = co_await state->lease->receive(state->inbound,
                                                        child_operation_options);
          if (received.status() != vix::CompletionStatus::Success)
          {
            (void)finish_handshake(state, true);
            if (received.status() == vix::CompletionStatus::Cancelled)
              parent->complete(vix::Completion<void>::cancelled());
            else
              parent->complete(vix::Completion<void>{received.error()});
            co_return;
          }
          const auto count = received.take();
          if (count == 0)
          {
            (void)finish_handshake(state, true);
            parent->complete(failure_completion(peer_truncated,
                                                "transport ended during TLS handshake"));
            co_return;
          }
          if (!retain_received_input(state, count))
          {
            (void)finish_handshake(state, true);
            parent->complete(failure_completion(backend_failure,
                                                "TLS handshake input could not be retained"));
            co_return;
          }
        }
      }
      catch (...)
      {
        (void)finish_handshake(state, engine_entered, false, !engine_entered);
        parent->complete(failure_completion(backend_failure,
                                            "exception escaped private TLS driver"));
      }
    }

    ParentFrame drive_read(
        std::shared_ptr<PrivateChannel::State> state,
        std::shared_ptr<vix::network::detail::OperationRecord<std::size_t>> parent,
        std::span<std::byte> output, vix::network::OperationOptions options,
        vix::CancellationSource cancellation)
    {
      auto finish = [&](vix::Completion<std::size_t> result, bool invalidate = false)
      {
        finish_parent(state, true, invalidate);
        parent->complete(std::move(result));
      };
      try
      {
        if (output.empty())
        {
          finish(vix::Completion<std::size_t>{std::size_t{0}});
          co_return;
        }
        auto cancellation_registration = options.cancellation.on_cancel(
            [cancellation] { (void)cancellation.request_cancel(); });
        const auto child = child_options(cancellation, options.deadline);
        std::array<std::byte, OpenSslDriver::buffer_capacity> ciphertext{};
        std::optional<AsyncGate::Guard> protocol_guard;
        bool retry = false;
        for (;;)
        {
          if (cancellation.is_cancel_requested())
          {
            finish(vix::Completion<std::size_t>::cancelled());
            co_return;
          }
          if (options.deadline && options.deadline->reached())
          {
            finish(failure_completion<std::size_t>(vix::network::timed_out,
                                                   "TLS read deadline reached"));
            co_return;
          }
          if (!protocol_guard)
          {
            auto acquired = co_await state->protocol_gate->lock(state->provider,
                                                               cancellation.token(),
                                                               options.deadline);
            if (!acquired)
            {
              if (cancellation.is_cancel_requested())
                finish(vix::Completion<std::size_t>::cancelled());
              else
                finish(failure_completion<std::size_t>(vix::network::timed_out,
                                                       "TLS read deadline reached"));
              co_return;
            }
            auto fed = feed_retained_input(state);
            if (!fed)
            {
              finish(failure_completion<std::size_t>(buffer_limit,
                                                     "TLS input buffer is full"), true);
              co_return;
            }
            protocol_guard.emplace(std::move(*acquired));
            if (cancellation.is_cancel_requested())
            {
              finish(vix::Completion<std::size_t>::cancelled());
              co_return;
            }
            if (options.deadline && options.deadline->reached())
            {
              finish(failure_completion<std::size_t>(vix::network::timed_out,
                                                     "TLS read deadline reached"));
              co_return;
            }
          }
          const auto input_generation = received_input_generation(state);
          auto progress = retry ? state->driver->retry_read(ciphertext)
                                : state->driver->read_step(ciphertext);
          retry = true;
          if (progress.ciphertext_bytes != 0)
          {
            auto send_guard = co_await state->send_gate->lock(state->provider,
                                                             cancellation.token(),
                                                             options.deadline);
            if (!send_guard)
            {
              if (cancellation.is_cancel_requested())
                finish(vix::Completion<std::size_t>::cancelled(), true);
              else
                finish(failure_completion<std::size_t>(vix::network::timed_out,
                                                       "TLS read deadline reached"), true);
              co_return;
            }
            auto sent = co_await state->lease->send(
                std::span<const std::byte>{ciphertext.data(), progress.ciphertext_bytes}, child);
            if (sent.status() != vix::CompletionStatus::Success)
            {
              if (sent.status() == vix::CompletionStatus::Cancelled)
                finish(vix::Completion<std::size_t>::cancelled(), true);
              else
                finish(vix::Completion<std::size_t>{sent.error()}, true);
              co_return;
            }
          }
          if (progress.want != EngineWant::write &&
              progress.want != EngineWant::failure &&
              progress.want != EngineWant::closed)
            protocol_guard.reset();
          if (progress.want == EngineWant::complete)
          {
            auto plaintext = state->driver->take_plaintext(output);
            if (!plaintext)
            {
              finish(failure_completion<std::size_t>(backend_failure,
                                                     "TLS plaintext result is unavailable"), true);
              co_return;
            }
            finish(vix::Completion<std::size_t>{plaintext.value()});
            co_return;
          }
          if (progress.want == EngineWant::closed)
          {
            {
              std::lock_guard lock(state->operation_mutex);
              state->peer_close_received = true;
            }
            finish(failure_completion<std::size_t>(peer_closed,
                                                   "TLS peer sent close_notify"));
            co_return;
          }
          if (progress.want == EngineWant::failure)
          {
            finish(failure_completion<std::size_t>(progress.error,
                                                   "TLS read backend failed"), true);
            co_return;
          }
          if (progress.want == EngineWant::write)
          {
            if (progress.ciphertext_bytes == 0)
            {
              finish(failure_completion<std::size_t>(backend_failure,
                                                     "TLS read requested unavailable output"), true);
              co_return;
            }
            continue;
          }

          auto receive_guard = co_await state->receive_gate->lock(state->provider,
                                                                 cancellation.token(),
                                                                 options.deadline);
          if (!receive_guard)
          {
            if (cancellation.is_cancel_requested())
              finish(vix::Completion<std::size_t>::cancelled());
            else
              finish(failure_completion<std::size_t>(vix::network::timed_out,
                                                     "TLS read deadline reached"));
            co_return;
          }
          if (received_input_generation(state) != input_generation)
          {
            receive_guard.reset();
            continue;
          }
          auto fed = feed_retained_input(state);
          if (!fed)
          {
            finish(failure_completion<std::size_t>(buffer_limit,
                                                   "TLS input buffer is full"), true);
            co_return;
          }
          if (retained_input_pending(state))
          {
            if (fed.value() == 0)
            {
              finish(failure_completion<std::size_t>(buffer_limit,
                                                     "TLS inbound BIO cannot accept more ciphertext"), true);
              co_return;
            }
            receive_guard.reset();
            continue;
          }
          auto received = co_await state->lease->receive(state->inbound, child);
          if (received.status() != vix::CompletionStatus::Success)
          {
            if (received.status() == vix::CompletionStatus::Cancelled)
              finish(vix::Completion<std::size_t>::cancelled());
            else
              finish(vix::Completion<std::size_t>{received.error()},
                     received.error().identity() != vix::network::timed_out);
            co_return;
          }
          const auto count = received.take();
          if (count == 0)
          {
            finish(failure_completion<std::size_t>(peer_truncated,
                                                   "transport EOF before TLS close_notify"), true);
            co_return;
          }
          if (!retain_received_input(state, count))
          {
            finish(failure_completion<std::size_t>(backend_failure,
                                                   "TLS input could not be retained"), true);
            co_return;
          }
        }
      }
      catch (...)
      {
        finish(failure_completion<std::size_t>(backend_failure,
                                               "exception escaped private TLS read"), true);
      }
    }

    ParentFrame drive_write(
        std::shared_ptr<PrivateChannel::State> state,
        std::shared_ptr<vix::network::detail::OperationRecord<void>> parent,
        std::span<const std::byte> input, vix::network::OperationOptions options,
        vix::CancellationSource cancellation)
    {
      auto finish = [&](vix::Completion<void> result, bool invalidate = false)
      {
        finish_parent(state, false, invalidate);
        parent->complete(std::move(result));
      };
      bool invoked = false;
      try
      {
        if (input.empty())
        {
          finish(vix::Completion<void>{vix::Result<void>::success()});
          co_return;
        }
        if (input.size() > OpenSslDriver::buffer_capacity)
        {
          finish(failure_completion<void>(buffer_limit,
                                         "TLS write exceeds the bounded plaintext capacity"));
          co_return;
        }
        auto cancellation_registration = options.cancellation.on_cancel(
            [cancellation] { (void)cancellation.request_cancel(); });
        const auto child = child_options(cancellation, options.deadline);
        std::array<std::byte, OpenSslDriver::buffer_capacity> ciphertext{};
        std::optional<AsyncGate::Guard> protocol_guard;
        bool retry = false;
        for (;;)
        {
          if (cancellation.is_cancel_requested())
          {
            finish(vix::Completion<void>::cancelled(), invoked);
            co_return;
          }
          if (options.deadline && options.deadline->reached())
          {
            finish(failure_completion<void>(vix::network::timed_out,
                                            "TLS write deadline reached"), invoked);
            co_return;
          }
          if (!protocol_guard)
          {
            auto acquired = co_await state->protocol_gate->lock(state->provider,
                                                               cancellation.token(),
                                                               options.deadline);
            if (!acquired)
            {
              if (cancellation.is_cancel_requested())
                finish(vix::Completion<void>::cancelled(), invoked);
              else
                finish(failure_completion<void>(vix::network::timed_out,
                                                "TLS write deadline reached"), invoked);
              co_return;
            }
            auto fed = feed_retained_input(state);
            if (!fed)
            {
              finish(failure_completion<void>(buffer_limit,
                                              "TLS input buffer is full"), true);
              co_return;
            }
            protocol_guard.emplace(std::move(*acquired));
            if (cancellation.is_cancel_requested())
            {
              finish(vix::Completion<void>::cancelled(), invoked);
              co_return;
            }
            if (options.deadline && options.deadline->reached())
            {
              finish(failure_completion<void>(vix::network::timed_out,
                                              "TLS write deadline reached"), invoked);
              co_return;
            }
          }
          const auto input_generation = received_input_generation(state);
          auto progress = retry ? state->driver->retry_write(ciphertext)
                                : state->driver->write_step(input, ciphertext);
          invoked = true;
          retry = true;
          if (progress.ciphertext_bytes != 0)
          {
            auto send_guard = co_await state->send_gate->lock(state->provider,
                                                             cancellation.token(),
                                                             options.deadline);
            if (!send_guard)
            {
              if (cancellation.is_cancel_requested())
                finish(vix::Completion<void>::cancelled(), true);
              else
                finish(failure_completion<void>(vix::network::timed_out,
                                                "TLS write deadline reached"), true);
              co_return;
            }
            auto sent = co_await state->lease->send(
                std::span<const std::byte>{ciphertext.data(), progress.ciphertext_bytes}, child);
            if (sent.status() != vix::CompletionStatus::Success)
            {
              if (sent.status() == vix::CompletionStatus::Cancelled)
                finish(vix::Completion<void>::cancelled(), true);
              else
                finish(vix::Completion<void>{sent.error()}, true);
              co_return;
            }
          }
          if (progress.want != EngineWant::write &&
              progress.want != EngineWant::failure &&
              progress.want != EngineWant::closed)
            protocol_guard.reset();
          if (progress.want == EngineWant::complete)
          {
            finish(vix::Completion<void>{vix::Result<void>::success()});
            co_return;
          }
          if (progress.want == EngineWant::closed ||
              progress.want == EngineWant::failure)
          {
            finish(failure_completion<void>(progress.error,
                                            "TLS write backend failed"), true);
            co_return;
          }
          if (progress.want == EngineWant::write)
          {
            if (progress.ciphertext_bytes == 0)
            {
              finish(failure_completion<void>(backend_failure,
                                              "TLS write requested unavailable output"), true);
              co_return;
            }
            continue;
          }

          auto receive_guard = co_await state->receive_gate->lock(state->provider,
                                                                 cancellation.token(),
                                                                 options.deadline);
          if (!receive_guard)
          {
            if (cancellation.is_cancel_requested())
              finish(vix::Completion<void>::cancelled(), invoked);
            else
              finish(failure_completion<void>(vix::network::timed_out,
                                              "TLS write deadline reached"), invoked);
            co_return;
          }
          if (received_input_generation(state) != input_generation)
          {
            receive_guard.reset();
            continue;
          }
          auto fed = feed_retained_input(state);
          if (!fed)
          {
            finish(failure_completion<void>(buffer_limit,
                                            "TLS input buffer is full"), true);
            co_return;
          }
          if (retained_input_pending(state))
          {
            if (fed.value() == 0)
            {
              finish(failure_completion<void>(buffer_limit,
                                              "TLS inbound BIO cannot accept more ciphertext"), true);
              co_return;
            }
            receive_guard.reset();
            continue;
          }
          auto received = co_await state->lease->receive(state->inbound, child);
          if (received.status() != vix::CompletionStatus::Success)
          {
            if (received.status() == vix::CompletionStatus::Cancelled)
              finish(vix::Completion<void>::cancelled(), invoked);
            else
              finish(vix::Completion<void>{received.error()}, true);
            co_return;
          }
          const auto count = received.take();
          if (count == 0)
          {
            finish(failure_completion<void>(peer_truncated,
                                            "transport EOF during TLS write"), true);
            co_return;
          }
          if (!retain_received_input(state, count))
          {
            finish(failure_completion<void>(backend_failure,
                                            "TLS input could not be retained"), true);
            co_return;
          }
        }
      }
      catch (...)
      {
        finish(failure_completion<void>(backend_failure,
                                        "exception escaped private TLS write"), true);
      }
    }

    ParentFrame drive_shutdown(
        std::shared_ptr<PrivateChannel::State> state,
        std::shared_ptr<vix::network::detail::OperationRecord<void>> parent,
        vix::network::OperationOptions options,
        vix::CancellationSource cancellation)
    {
      auto finish = [&](vix::Completion<void> result)
      {
        auto transport_result = finish_shutdown(state);
        if (result.status() == vix::CompletionStatus::Success && !transport_result)
        {
          parent->complete(vix::Completion<void>{transport_result.error()});
          return;
        }
        parent->complete(std::move(result));
      };
      try
      {
        auto cancellation_registration = options.cancellation.on_cancel(
            [cancellation] { (void)cancellation.request_cancel(); });
        const auto child = child_options(cancellation, options.deadline);
        std::array<std::byte, OpenSslDriver::buffer_capacity> ciphertext{};
        std::optional<AsyncGate::Guard> protocol_guard;
        bool retry = false;
        for (;;)
        {
          if (cancellation.is_cancel_requested())
          {
            finish(vix::Completion<void>::cancelled());
            co_return;
          }
          if (options.deadline && options.deadline->reached())
          {
            finish(failure_completion<void>(vix::network::timed_out,
                                            "TLS shutdown deadline reached"));
            co_return;
          }
          if (!protocol_guard)
          {
            auto acquired = co_await state->protocol_gate->lock(
                state->provider, cancellation.token(), options.deadline);
            if (!acquired)
            {
              if (cancellation.is_cancel_requested())
                finish(vix::Completion<void>::cancelled());
              else
                finish(failure_completion<void>(vix::network::timed_out,
                                                "TLS shutdown deadline reached"));
              co_return;
            }
            protocol_guard.emplace(std::move(*acquired));
          }

          // A receive child retains ciphertext outside OpenSSL until it is
          // explicitly fed to the inbound BIO. Shutdown keeps the protocol
          // gate across WANT_READ, so this must run on every retry, not only
          // when the gate is first acquired. Otherwise SSL_shutdown retries
          // against an empty BIO and may read TCP EOF before consuming an
          // already-received peer close_notify.
          auto fed = feed_retained_input(state);
          if (!fed)
          {
            finish(failure_completion<void>(buffer_limit,
                                            "TLS shutdown input buffer failed"));
            co_return;
          }

          auto progress = retry ? state->driver->retry_shutdown(ciphertext)
                                : state->driver->shutdown_step(ciphertext);
          retry = true;
          if (progress.ciphertext_bytes != 0)
            state->shutdown_ciphertext_generated.fetch_add(
                progress.ciphertext_bytes, std::memory_order_release);
          if (progress.ciphertext_bytes != 0)
          {
            auto send_guard = co_await state->send_gate->lock(
                state->provider, cancellation.token(), options.deadline);
            if (!send_guard)
            {
              finish(cancellation.is_cancel_requested()
                         ? vix::Completion<void>::cancelled()
                         : failure_completion<void>(vix::network::timed_out,
                                                   "TLS shutdown deadline reached"));
              co_return;
            }
            auto sent = co_await state->lease->send(
                std::span<const std::byte>{ciphertext.data(), progress.ciphertext_bytes}, child);
            if (sent.status() != vix::CompletionStatus::Success)
            {
              finish(sent.status() == vix::CompletionStatus::Cancelled
                         ? vix::Completion<void>::cancelled()
                         : vix::Completion<void>{sent.error()});
              co_return;
            }
            state->shutdown_ciphertext_sent.fetch_add(
                progress.ciphertext_bytes, std::memory_order_release);
          }
          if (progress.want == EngineWant::complete)
          {
            finish(vix::Completion<void>{vix::Result<void>::success()});
            co_return;
          }
          if (progress.want == EngineWant::failure || progress.want == EngineWant::closed)
          {
            finish(failure_completion<void>(progress.error,
                                            "TLS close_notify exchange failed"));
            co_return;
          }
          if (progress.want == EngineWant::retry)
            continue;
          if (progress.want == EngineWant::write)
          {
            if (progress.ciphertext_bytes == 0)
            {
              finish(failure_completion<void>(backend_failure,
                                              "TLS shutdown requested unavailable output"));
              co_return;
            }
            continue;
          }

          auto receive_guard = co_await state->receive_gate->lock(
              state->provider, cancellation.token(), options.deadline);
          if (!receive_guard)
          {
            finish(cancellation.is_cancel_requested()
                       ? vix::Completion<void>::cancelled()
                       : failure_completion<void>(vix::network::timed_out,
                                                 "TLS shutdown deadline reached"));
            co_return;
          }
          fed = feed_retained_input(state);
          if (!fed)
          {
            finish(failure_completion<void>(buffer_limit,
                                            "TLS shutdown input buffer failed"));
            co_return;
          }
          if (retained_input_pending(state))
          {
            if (fed.value() == 0)
            {
              finish(failure_completion<void>(buffer_limit,
                                              "TLS shutdown input exceeds BIO capacity"));
              co_return;
            }
            receive_guard.reset();
            continue;
          }
          auto received = co_await state->lease->receive(state->inbound, child);
          if (received.status() != vix::CompletionStatus::Success)
          {
            finish(received.status() == vix::CompletionStatus::Cancelled
                       ? vix::Completion<void>::cancelled()
                       : vix::Completion<void>{received.error()});
            co_return;
          }
          const auto count = received.take();
          if (count == 0)
          {
            finish(failure_completion<void>(peer_truncated,
                                            "transport EOF before peer close_notify"));
            co_return;
          }
          if (!retain_received_input(state, count))
          {
            finish(failure_completion<void>(backend_failure,
                                            "TLS shutdown input could not be retained"));
            co_return;
          }
        }
      }
      catch (...)
      {
        // Once SSL_shutdown has been entered, protocol state cannot be reused.
        finish(failure_completion<void>(backend_failure,
                                        "exception escaped private TLS shutdown"));
      }
    }
  }

  vix::Result<std::shared_ptr<PrivateChannel>> PrivateChannel::create(
      vix::network::detail::TcpTransportLease lease, SSL_CTX *context,
      bool server_role)
  {
    auto driver = OpenSslDriver::create(context, server_role);
    if (!driver)
      return driver.error();
    return create(std::move(lease), driver.take());
  }

  vix::Result<std::shared_ptr<PrivateChannel>> PrivateChannel::create(
      vix::network::detail::TcpTransportLease lease,
      std::unique_ptr<OpenSslDriver> driver)
  {
    auto channel = prepare(std::move(driver));
    if (!channel)
      return channel.error();
    if (!channel.value()->attach(std::move(lease)))
      return vix::Error{channel_closed, "TLS transport lease is empty"};
    return channel.take();
  }

  vix::Result<std::shared_ptr<PrivateChannel>> PrivateChannel::prepare(
      std::unique_ptr<OpenSslDriver> driver)
  {
    if (!driver)
      return vix::Error{backend_failure, "TLS backend is unavailable"};
    auto state = std::make_shared<State>(std::move(driver));
    auto *channel = new (std::nothrow) PrivateChannel{std::move(state)};
    if (channel == nullptr)
      return vix::Error{resource_limit, "TLS channel allocation failed"};
    return std::shared_ptr<PrivateChannel>{channel};
  }

  bool PrivateChannel::attach(
      vix::network::detail::TcpTransportLease lease) noexcept
  {
    auto provider = vix::network::detail::NetworkAccess::provider_for_tls_lease(lease);
    if (!provider || !state_ || state_->lease)
      return false;
    state_->lease.emplace(std::move(lease));
    state_->provider = std::move(provider);
    return true;
  }

  PrivateChannel::~PrivateChannel()
  {
    if (state_)
      request_channel_close(state_);
  }

  void PrivateChannel::close() noexcept
  {
    if (state_)
      request_channel_close(state_);
  }

  PrivateChannel::ShutdownDiagnostics
  PrivateChannel::shutdown_diagnostics_for_test() const noexcept
  {
    if (!state_ || !state_->driver)
      return {};
    return {state_->driver->shutdown_diagnostics_for_test(),
            state_->shutdown_ciphertext_generated.load(std::memory_order_acquire),
            state_->shutdown_ciphertext_sent.load(std::memory_order_acquire)};
  }

  std::shared_ptr<vix::network::detail::ProviderCore>
  PrivateChannel::provider_for_http_composition() const noexcept
  {
    return state_ ? state_->provider : nullptr;
  }

  vix::network::Operation<void> PrivateChannel::handshake(
      vix::CancellationToken cancellation, std::optional<vix::Deadline> deadline)
  {
    auto state = state_;
    vix::network::OperationOptions options{std::move(cancellation), deadline};
    return vix::network::detail::OperationFactory<void>::make(
        state->provider,
        [state, options = std::move(options)](auto parent) mutable
        {
          if (wrong_cooperative_thread(state))
          {
            parent->complete(failure_completion<void>(
                vix::network::operation_in_progress,
                "cooperative TLS operation is thread-affine"));
            return;
          }
          vix::CancellationSource source;
          bool unavailable = false;
          bool already_started = false;
          {
            std::lock_guard lock(state->operation_mutex);
            already_started = state->handshake_started;
            unavailable = state->invalidated;
            if (!already_started && !unavailable)
            {
              state->handshake_started = true;
              state->handshake_active = true;
              state->handshake_cancellation.emplace(source);
            }
          }
          if (unavailable || already_started)
          {
            parent->complete(failure_completion<void>(
                unavailable ? channel_closed : operation_conflict,
                unavailable ? "TLS transport is no longer available"
                            : "TLS handshake already started"));
            return;
          }
          try
          {
            (void)drive_handshake(state, parent, options, source);
          }
          catch (...)
          {
            (void)finish_handshake(state, false, false, true);
            parent->complete(failure_completion(backend_failure,
                                                "cannot allocate TLS parent coroutine state"));
          }
        });
  }

  vix::network::Operation<std::size_t> PrivateChannel::read(
      std::span<std::byte> output, vix::CancellationToken cancellation,
      std::optional<vix::Deadline> deadline)
  {
    auto state = state_;
    vix::network::OperationOptions options{std::move(cancellation), deadline};
    return vix::network::detail::OperationFactory<std::size_t>::make(
        state->provider,
        [state, output, options = std::move(options)](auto parent) mutable
        {
          if (wrong_cooperative_thread(state))
          {
            parent->complete(failure_completion<std::size_t>(
                vix::network::operation_in_progress,
                "cooperative TLS operation is thread-affine"));
            return;
          }
          vix::CancellationSource source;
          bool channel_unavailable = false;
          bool operation_busy = false;
          bool peer_closed_already = false;
          {
            std::lock_guard lock(state->operation_mutex);
            if (!state->handshake_complete || state->invalidated ||
                state->shutdown_started)
              channel_unavailable = true;
            else if (state->peer_close_received)
              peer_closed_already = true;
            else if (state->read_active)
              operation_busy = true;
            else
            {
              state->read_active = true;
              state->read_cancellation.emplace(source);
            }
          }
          if (channel_unavailable || peer_closed_already || operation_busy)
          {
            parent->complete(failure_completion<std::size_t>(
                channel_unavailable ? channel_closed
                                    : peer_closed_already ? peer_closed : operation_conflict,
                channel_unavailable ? "TLS channel is not open"
                                    : peer_closed_already ? "TLS peer sent close_notify"
                                                          : "TLS read is already active"));
            return;
          }
          try
          {
            (void)drive_read(state, parent, output, std::move(options), source);
          }
          catch (...)
          {
            finish_parent(state, true, true);
            parent->complete(failure_completion<std::size_t>(backend_failure,
                                                             "cannot allocate TLS read state"));
          }
        });
  }

  vix::network::Operation<void> PrivateChannel::write(
      std::span<const std::byte> input, vix::CancellationToken cancellation,
      std::optional<vix::Deadline> deadline)
  {
    auto state = state_;
    vix::network::OperationOptions options{std::move(cancellation), deadline};
    return vix::network::detail::OperationFactory<void>::make(
        state->provider,
        [state, input, options = std::move(options)](auto parent) mutable
        {
          if (wrong_cooperative_thread(state))
          {
            parent->complete(failure_completion<void>(
                vix::network::operation_in_progress,
                "cooperative TLS operation is thread-affine"));
            return;
          }
          vix::CancellationSource source;
          bool channel_unavailable = false;
          bool operation_busy = false;
          {
            std::lock_guard lock(state->operation_mutex);
            if (!state->handshake_complete || state->invalidated ||
                state->shutdown_started)
              channel_unavailable = true;
            else if (state->write_active)
              operation_busy = true;
            else
            {
              state->write_active = true;
              state->write_cancellation.emplace(source);
            }
          }
          if (channel_unavailable || operation_busy)
          {
            parent->complete(failure_completion<void>(
                channel_unavailable ? channel_closed : operation_conflict,
                channel_unavailable ? "TLS channel is not open" : "TLS write is already active"));
            return;
          }
          try
          {
            (void)drive_write(state, parent, input, std::move(options), source);
          }
          catch (...)
          {
            finish_parent(state, false, true);
            parent->complete(failure_completion<void>(backend_failure,
                                                      "cannot allocate TLS write state"));
          }
        });
  }

  vix::network::Operation<void> PrivateChannel::shutdown(
      vix::CancellationToken cancellation, std::optional<vix::Deadline> deadline)
  {
    auto state = state_;
    vix::network::OperationOptions options{std::move(cancellation), deadline};
    return vix::network::detail::OperationFactory<void>::make(
        state->provider,
        [state, options = std::move(options)](auto parent) mutable
        {
          if (wrong_cooperative_thread(state))
          {
            parent->complete(failure_completion<void>(
                vix::network::operation_in_progress,
                "cooperative TLS operation is thread-affine"));
            return;
          }
          vix::CancellationSource source;
          bool unavailable = false;
          bool busy = false;
          {
            std::lock_guard lock(state->operation_mutex);
            if (!state->handshake_complete || state->invalidated)
              unavailable = true;
            else if (state->shutdown_started || state->read_active ||
                     state->write_active)
              busy = true;
            else
            {
              state->shutdown_started = true;
              state->shutdown_active = true;
              state->shutdown_cancellation.emplace(source);
            }
          }
          if (unavailable || busy)
          {
            parent->complete(failure_completion<void>(
                unavailable ? channel_closed : operation_conflict,
                unavailable ? "TLS channel is not open"
                            : "TLS stream operation prevents shutdown"));
            return;
          }
          try
          {
            (void)drive_shutdown(state, parent, options, source);
          }
          catch (...)
          {
            (void)finish_shutdown(state);
            parent->complete(failure_completion<void>(
                backend_failure, "cannot allocate TLS shutdown state"));
          }
        });
  }
}
