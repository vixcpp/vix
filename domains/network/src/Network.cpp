#include "Engine.hpp"
#include "SocketPlatform.hpp"
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Errors.hpp>
#include <optional>
#include <limits>
#include <utility>

namespace vix::network::detail
{
  vix::Error transport_error_for_test(int error) noexcept { return transport_error(error, "test"); }
  #ifdef VIX_NETWORK_TEST_CAPTURE
  namespace
  {
    thread_local const TcpConnectionState *read_capture_connection_for_test{};
    thread_local std::function<void(std::shared_ptr<OperationRecord<std::size_t>>)> read_capture_for_test;
    thread_local const TcpConnectionState *write_capture_connection_for_test{};
    thread_local std::function<void(std::shared_ptr<OperationRecord<void>>)> write_capture_for_test;
    std::function<void(std::shared_ptr<OperationRecord<std::size_t>>)> take_read_capture_for_test(const TcpConnectionState *connection)
    {
      if (read_capture_connection_for_test != connection)
        return {};
      read_capture_connection_for_test = nullptr;
      return std::move(read_capture_for_test);
    }
    std::function<void(std::shared_ptr<OperationRecord<void>>)> take_write_capture_for_test(const TcpConnectionState *connection)
    {
      if (write_capture_connection_for_test != connection)
        return {};
      write_capture_connection_for_test = nullptr;
      return std::move(write_capture_for_test);
    }
  }
  #endif
  class TcpConnectionState : public std::enable_shared_from_this<TcpConnectionState>
  {
  public:
    TcpConnectionState(NativeSocket fd, std::shared_ptr<ProviderCore> provider) noexcept : fd_(fd), provider_(provider), lease_(std::in_place, std::move(provider)) {}
    TcpConnectionState(NativeSocket fd, ProviderResourceLease lease) noexcept : fd_(fd), provider_(lease.provider()), lease_(std::in_place, std::move(lease)) {}
    ~TcpConnectionState()
    {
      if (provider_->cooperative() && !provider_->on_owner_thread())
        std::terminate();
      (void)close();
    }
    vix::Result<void> close() noexcept
    {
      if (provider_->cooperative() && !provider_->on_owner_thread())
        return vix::Error{operation_in_progress, "cooperative connection is thread-affine"};
      std::lock_guard lock(mutex_);
      if (closed_)
        return error_ ? vix::Result<void>{*error_} : vix::Result<void>::success();
      if (read_active_ || write_active_)
        return vix::Error{operation_in_progress, "transport operation is active"};
      closed_ = true;
      if (descriptor_)
        provider_->retire(std::exchange(descriptor_, 0));
      const NativeSocket fd = std::exchange(fd_, invalid_socket);
      if (fd != invalid_socket && close_native_socket(fd) != 0)
        error_.emplace(vix::Error{close_failed, "close"});
      lease_.reset();
      return error_ ? vix::Result<void>{*error_} : vix::Result<void>::success();
    }
    vix::Result<void> shutdown_write() noexcept
    {
      if (provider_->cooperative() && !provider_->on_owner_thread())
        return vix::Error{operation_in_progress, "cooperative connection is thread-affine"};
      std::lock_guard lock(mutex_);
      if (closed_)
        return vix::Error{peer_closed,"connection is closed"};
      if (write_active_)
        return vix::Error{operation_in_progress,"write operation is active"};
      if (write_shutdown_)
        return vix::Result<void>::success();
      for (;;)
      {
        if (shutdown_socket_write(fd_) == 0)
        {
          write_shutdown_=true;
          return vix::Result<void>::success();
        }
        const int error = last_socket_error();
        if (socket_error_interrupted(error))
          continue;
        return transport_error(error, "shutdown write");
      }
    }
    int native_fd_for_test() const noexcept { return static_cast<int>(fd_); }
    vix::network::Operation<std::size_t> receive(std::span<std::byte> buffer, OperationOptions options);
    vix::network::Operation<void> send(std::span<const std::byte> buffer, OperationOptions options);

  private:
    friend class NetworkAccess;
    template <class T>
    void release_direction(bool read)
    {
      std::lock_guard lock(mutex_);
      if (read)
      {
        read_active_ = false;
        read_record_.reset();
        read_buffer_ = {};
        ++read_slot_releases_for_test_;
      }
      else
      {
        write_active_ = false;
        write_record_.reset();
        write_buffer_ = {};
        write_offset_at_terminal_release_for_test_ = write_offset_;
        write_offset_ = 0;
        ++write_slot_releases_for_test_;
      }
      update_interest_locked();
    }
    void update_interest_locked()
    {
      if (!descriptor_)
        return;
      if (!read_active_ && !write_active_)
      {
        provider_->retire(std::exchange(descriptor_, 0));
        return;
      }
      auto weak = weak_from_this();
      (void)provider_->set_descriptor_handlers(descriptor_,
                                               read_active_ ? ReadinessHandler{[weak](std::uint32_t)
                                                                               { if(auto self=weak.lock()) { self->progress_receive(); return true; } return false; }}
                                                            : ReadinessHandler{},
                                               write_active_ ? ReadinessHandler{[weak](std::uint32_t)
                                                                                { if(auto self=weak.lock()) { self->progress_send(); return true; } return false; }}
                                                             : ReadinessHandler{});
    }
    void progress_receive();
    void progress_send();
    bool ensure_descriptor_locked();
    void hold_progression_for_test(bool hold) noexcept
    {
      std::lock_guard lock(mutex_);
      progression_held_for_test_ = hold;
    }
    bool post_read_progression_for_test()
    {
      auto self = shared_from_this();
      return provider_->post([self] { self->progress_receive(); });
    }
    bool post_write_progression_for_test()
    {
      auto self = shared_from_this();
      return provider_->post([self] { self->progress_send(); });
    }
    void publish_receive_for_test(vix::Completion<std::size_t> completion)
    {
      std::shared_ptr<OperationRecord<std::size_t>> record;
      {
        std::lock_guard lock(mutex_);
        record = read_record_.lock();
      }
      if (record)
        record->publish_native_for_test(std::move(completion));
    }
    void publish_send_for_test(vix::Completion<void> completion)
    {
      if (completion.status() == vix::CompletionStatus::Success)
      {
        std::lock_guard lock(mutex_);
        write_offset_ = write_buffer_.size();
      }
      std::shared_ptr<OperationRecord<void>> record;
      {
        std::lock_guard lock(mutex_);
        record = write_record_.lock();
      }
      if (record)
        record->publish_native_for_test(std::move(completion));
    }
    void fail_descriptor_arm_for_test(bool fail) noexcept
    {
      std::lock_guard lock(mutex_);
      fail_descriptor_arm_for_test_ = fail;
    }
    NetworkAccess::TransportCountersForTest transport_counters_for_test()
    {
      std::lock_guard lock(mutex_);
      return {read_progressions_for_test_, write_progressions_for_test_, native_recv_attempts_for_test_, native_send_attempts_for_test_,
              read_buffer_accesses_for_test_, write_buffer_accesses_for_test_, read_slot_releases_for_test_, write_slot_releases_for_test_, write_offset_, write_offset_at_terminal_release_for_test_};
    }
    NativeSocket fd_{invalid_socket};
    std::shared_ptr<ProviderCore> provider_;
    bool closed_{}, write_shutdown_{};
    bool read_active_{}, write_active_{};
    std::weak_ptr<OperationRecord<std::size_t>> read_record_;
    std::weak_ptr<OperationRecord<void>> write_record_;
    std::span<std::byte> read_buffer_{};
    std::span<const std::byte> write_buffer_{};
    std::size_t write_offset_{};
    std::uint64_t descriptor_{};
    std::mutex mutex_;
    std::optional<ProviderResourceLease> lease_;
    std::optional<vix::Error> error_;
    bool progression_held_for_test_{};
    bool fail_descriptor_arm_for_test_{};
    std::uint64_t read_progressions_for_test_{}, write_progressions_for_test_{};
    std::uint64_t native_recv_attempts_for_test_{}, native_send_attempts_for_test_{};
    std::uint64_t read_buffer_accesses_for_test_{}, write_buffer_accesses_for_test_{};
    std::uint64_t read_slot_releases_for_test_{}, write_slot_releases_for_test_{};
    std::size_t write_offset_at_terminal_release_for_test_{};
  };

  class TcpListenerState : public std::enable_shared_from_this<TcpListenerState>
  {
  public:
    TcpListenerState(NativeSocket fd, Endpoint endpoint, std::shared_ptr<ProviderCore> provider) noexcept
      : fd_(fd), endpoint_(std::move(endpoint)), provider_(std::move(provider)), lease_(std::in_place, provider_) {}
    ~TcpListenerState()
    {
      if (provider_->cooperative() && !provider_->on_owner_thread())
        std::terminate();
      (void)close();
    }

    vix::Result<Endpoint> local_endpoint() const
    {
      std::lock_guard lock(mutex_);
      if (closed_)
        return vix::Error{peer_closed, "listener is closed"};
      return endpoint_;
    }
    vix::Result<void> close() noexcept
    {
      if (provider_->cooperative() && !provider_->on_owner_thread())
        return vix::Error{operation_in_progress, "cooperative listener is thread-affine"};
      std::uint64_t descriptor{};
      NativeSocket fd{invalid_socket};
      {
        std::lock_guard lock(mutex_);
        if (closed_)
          return error_ ? vix::Result<void>{*error_} : vix::Result<void>::success();
        if (accept_active_)
          return vix::Error{operation_in_progress, "accept operation is active"};
        closed_ = true;
        descriptor = std::exchange(descriptor_, 0);
        fd = std::exchange(fd_, invalid_socket);
      }
      if (descriptor)
        provider_->retire(descriptor);
      if (fd != invalid_socket && close_native_socket(fd) != 0)
        error_.emplace(vix::Error{close_failed, "close listener"});
      lease_.reset();
      return error_ ? vix::Result<void>{*error_} : vix::Result<void>::success();
    }

    vix::network::Operation<TcpConnection> accept(OperationOptions options);

  private:
    void release_accept()
    {
      std::uint64_t descriptor{};
      {
        std::lock_guard lock(mutex_);
        accept_active_ = false;
        accept_result_published_ = false;
        accept_record_.reset();
        ++accept_generation_;
        descriptor = std::exchange(descriptor_, 0);
      }
      if (descriptor)
        provider_->retire(descriptor);
    }
    void progress_accept(std::uint64_t generation)
    {
      std::shared_ptr<OperationRecord<TcpConnection>> record;
      NativeSocket fd{invalid_socket};
      {
        std::lock_guard lock(mutex_);
        if (closed_ || generation != accept_generation_ || !accept_active_ || accept_result_published_)
          return;
        record = accept_record_.lock();
        fd = fd_;
      }
      if (!record)
        return;
      for (;;)
      {
        int error{};
        const NativeSocket accepted = accept_tcp_socket(fd, error);
        if (accepted != invalid_socket)
        {
          {
            std::lock_guard lock(mutex_);
            if (closed_ || generation != accept_generation_ || !accept_active_ || accept_result_published_ || accept_record_.lock() != record)
            {
              (void)close_native_socket(accepted);
              return;
            }
            accept_result_published_ = true;
          }
          if (!record->adopt_native_for_test(accepted))
          {
            (void)close_native_socket(accepted);
            std::terminate();
          }
          record->publish_native_success_with([](NativeSocket client_fd, ProviderResourceLease lease)
                                              { return make_tcp_connection(client_fd, std::move(lease)); });
          return;
        }
        if (socket_error_accept_retry(error))
          continue;
        if (socket_error_would_block(error))
          return;
        {
          std::lock_guard lock(mutex_);
          if (generation != accept_generation_ || !accept_active_ || accept_result_published_ || accept_record_.lock() != record)
            return;
          accept_result_published_ = true;
        }
        record->publish_native_for_test(vix::Completion<TcpConnection>{transport_error(error, "accept")});
        return;
      }
    }

    NativeSocket fd_{invalid_socket};
    Endpoint endpoint_;
    std::shared_ptr<ProviderCore> provider_;
    mutable std::mutex mutex_;
    bool closed_{}, accept_active_{}, accept_result_published_{};
    std::weak_ptr<OperationRecord<TcpConnection>> accept_record_;
    std::uint64_t descriptor_{};
    std::uint64_t accept_generation_{};
    std::optional<ProviderResourceLease> lease_;
    std::optional<vix::Error> error_;
  };

  bool TcpConnectionState::ensure_descriptor_locked()
  {
    if (descriptor_)
      return true;
    if (fail_descriptor_arm_for_test_)
      return false;
    auto weak = weak_from_this();
    descriptor_ = provider_->register_descriptor(fd_,
                                                 read_active_ ? ReadinessHandler{[weak](std::uint32_t)
                                                                                 { if(auto self=weak.lock()) { self->progress_receive(); return true; } return false; }}
                                                              : ReadinessHandler{},
                                                 write_active_ ? ReadinessHandler{[weak](std::uint32_t)
                                                                                  { if(auto self=weak.lock()) { self->progress_send(); return true; } return false; }}
                                                               : ReadinessHandler{});
    return descriptor_ != 0;
  }
  void TcpConnectionState::progress_receive()
  {
    std::shared_ptr<OperationRecord<std::size_t>> record;
    std::span<std::byte> buffer;
    NativeSocket fd;
    {
      std::lock_guard lock(mutex_);
      ++read_progressions_for_test_;
      if (progression_held_for_test_)
        return;
      if (!read_active_ || closed_)
        return;
      record = read_record_.lock();
      buffer = read_buffer_;
      fd = fd_;
    }
    for (;;)
    {
      {
        std::lock_guard lock(mutex_);
        if (!read_active_ || closed_)
          return;
        ++native_recv_attempts_for_test_;
        ++read_buffer_accesses_for_test_;
      }
      const auto result = receive_socket(fd, buffer);
      if (result >= 0)
      {
        record->publish_native_for_test(vix::Completion<std::size_t>{static_cast<std::size_t>(result)});
        return;
      }
      const int error = last_socket_error();
      if (socket_error_interrupted(error))
        continue;
      if (socket_error_would_block(error))
        return;
      record->publish_native_for_test(vix::Completion<std::size_t>{transport_error(error, "recv")});
      return;
    }
  }
  void TcpConnectionState::progress_send()
  {
    std::shared_ptr<OperationRecord<void>> record;
    std::span<const std::byte> buffer;
    std::size_t offset;
    NativeSocket fd;
    {
      std::lock_guard lock(mutex_);
      ++write_progressions_for_test_;
      if (progression_held_for_test_)
        return;
      if (!write_active_ || closed_)
        return;
      record = write_record_.lock();
      buffer = write_buffer_;
      offset = write_offset_;
      fd = fd_;
    }
    // A readiness notification and the starter command can both be queued.
    // Once one has advanced the complete span, later callbacks are inert.
    if (offset == buffer.size())
      return;
    for (;;)
    {
      {
        std::lock_guard lock(mutex_);
        if (!write_active_ || closed_)
          return;
        ++native_send_attempts_for_test_;
        ++write_buffer_accesses_for_test_;
      }
      const auto result = send_socket(fd, buffer.subspan(offset));
      if (result > 0)
      {
        offset += static_cast<std::size_t>(result);
        {
          std::lock_guard lock(mutex_);
          if (!write_active_)
            return;
          write_offset_ = offset;
        }
        if (offset == buffer.size())
        {
          record->publish_native_for_test(vix::Completion<void>::success());
          return;
        }
        continue;
      }
      const int error = last_socket_error();
      if (result < 0 && socket_error_interrupted(error))
        continue;
      if (result < 0 && socket_error_would_block(error))
        return;
      record->publish_native_for_test(vix::Completion<void>{transport_error(result < 0 ? error : socket_error_peer_closed_code(), "send")});
      return;
    }
  }
  vix::network::Operation<std::size_t> TcpConnectionState::receive(std::span<std::byte> buffer, OperationOptions options)
  {
    auto self = shared_from_this();
#ifdef VIX_NETWORK_TEST_CAPTURE
    auto capture = take_read_capture_for_test(self.get());
    return OperationFactory<std::size_t>::make_with_capture(provider_, [self, buffer, options](auto record) mutable
#else
    return OperationFactory<std::size_t>::make(provider_, [self, buffer, options](auto record) mutable
#endif
                                               {
      if (self->provider_->cooperative() && !self->provider_->on_owner_thread())
      { record->complete(vix::Completion<std::size_t>{vix::Error{operation_in_progress, "cooperative connection is thread-affine"}}); return; }
      if(buffer.empty()) { record->complete(vix::Completion<std::size_t>{std::size_t{0}}); return; }
      bool closed = false, arm_failed = false;
      { std::lock_guard lock(self->mutex_);
        if(self->closed_) closed = true;
        else {
        if(self->read_active_) std::terminate();
        self->read_active_=true; self->read_record_=record; self->read_buffer_=buffer;
        record->set_terminal_cleanup([self]{ self->release_direction<std::size_t>(true); });
        arm_failed = !self->ensure_descriptor_locked();
        }
      }
      if(closed) { record->complete(vix::Completion<std::size_t>{vix::Error{peer_closed,"connection is closed"}}); return; }
      if(arm_failed) { record->complete(vix::Completion<std::size_t>{vix::Error{io_failure,"cannot arm receive"}}); return; }
      record->observe_cancellation(options.cancellation); if(options.deadline) record->observe_deadline(*options.deadline);
      if(!self->provider_->post([self]{self->progress_receive();})) record->complete(vix::Completion<std::size_t>{vix::Error{provider_stopped,"provider is stopped"}})
#ifdef VIX_NETWORK_TEST_CAPTURE
      ; },std::move(capture));
#else
      ; });
#endif
  }
  vix::network::Operation<void> TcpConnectionState::send(std::span<const std::byte> buffer, OperationOptions options)
  {
    auto self = shared_from_this();
#ifdef VIX_NETWORK_TEST_CAPTURE
    auto capture = take_write_capture_for_test(self.get());
    return OperationFactory<void>::make_with_capture(provider_, [self, buffer, options](auto record) mutable
#else
    return OperationFactory<void>::make(provider_, [self, buffer, options](auto record) mutable
#endif
                                        {
      if (self->provider_->cooperative() && !self->provider_->on_owner_thread())
      { record->complete(vix::Completion<void>{vix::Error{operation_in_progress, "cooperative connection is thread-affine"}}); return; }
      bool closed = false, write_shutdown = false, arm_failed = false;
      { std::lock_guard lock(self->mutex_);
        if(self->closed_) closed = true;
        else if(self->write_shutdown_) write_shutdown = true;
        else if(buffer.empty()) { }
        else {
        if(self->write_active_) std::terminate();
        self->write_active_=true; self->write_record_=record; self->write_buffer_=buffer; self->write_offset_=0;
        record->set_terminal_cleanup([self]{ self->release_direction<void>(false); });
        arm_failed = !self->ensure_descriptor_locked();
        }
      }
      if(closed) { record->complete(vix::Completion<void>{vix::Error{peer_closed,"connection is closed"}}); return; }
      if(write_shutdown) { record->complete(vix::Completion<void>{vix::Error{vix::network::write_shutdown,"connection write direction is shut down"}}); return; }
      if(buffer.empty()) { record->complete(vix::Completion<void>::success()); return; }
      if(arm_failed) { record->complete(vix::Completion<void>{vix::Error{io_failure,"cannot arm send"}}); return; }
      record->observe_cancellation(options.cancellation); if(options.deadline) record->observe_deadline(*options.deadline);
      if(!self->provider_->post([self]{self->progress_send();})) record->complete(vix::Completion<void>{vix::Error{provider_stopped,"provider is stopped"}})
#ifdef VIX_NETWORK_TEST_CAPTURE
      ; },std::move(capture));
#else
      ; });
#endif
  }
  vix::network::Operation<TcpConnection> TcpListenerState::accept(OperationOptions options)
  {
    auto self = shared_from_this();
    return OperationFactory<TcpConnection>::make(provider_, [self, options](auto record) mutable
                                                 {
      if (self->provider_->cooperative() && !self->provider_->on_owner_thread())
      { record->complete(vix::Completion<TcpConnection>{vix::Error{operation_in_progress, "cooperative listener is thread-affine"}}); return; }
      bool closed = false;
      bool arm_failed = false;
      {
        std::lock_guard lock(self->mutex_);
        closed = self->closed_;
        if (!closed)
        {
          if (self->accept_active_)
            std::terminate();
          self->accept_active_ = true;
          self->accept_result_published_ = false;
          self->accept_record_ = record;
          const auto generation = ++self->accept_generation_;
          record->set_terminal_cleanup([self] { self->release_accept(); });
          auto weak = self->weak_from_this();
          self->descriptor_ = self->provider_->register_descriptor(
              self->fd_, ReadinessHandler{[weak, generation](std::uint32_t)
                                          { if (auto listener = weak.lock()) { listener->progress_accept(generation); return true; } return false; }},
              {});
          arm_failed = !self->descriptor_;
          if (!arm_failed)
            record->bind_descriptor_direction(self->descriptor_, true);
        }
      }
      if (closed)
      {
        record->complete(vix::Completion<TcpConnection>{vix::Error{peer_closed, "listener is closed"}});
        return;
      }
      if (arm_failed)
      {
        record->complete(vix::Completion<TcpConnection>{vix::Error{io_failure, "cannot arm accept"}});
        return;
      }
      record->observe_cancellation(options.cancellation);
      if (options.deadline)
        record->observe_deadline(*options.deadline);
      std::uint64_t generation{};
      { std::lock_guard lock(self->mutex_); generation = self->accept_generation_; }
      if (!self->provider_->post([self, generation] { self->progress_accept(generation); }))
        record->complete(vix::Completion<TcpConnection>{vix::Error{provider_stopped, "provider is stopped"}});
    });
  }
}

namespace vix::network
{
  struct detail_native_endpoint
  {
    static detail::NativeAddress make(const Endpoint &endpoint) noexcept
    {
      return detail::make_native_address(endpoint.address_.family_, endpoint.address_.bytes_,
                                         endpoint.address_.scope_id_, endpoint.port_);
    }
  };
}

namespace vix::network
{
  namespace
  {
    void publish_connect_result(const std::shared_ptr<detail::OperationRecord<TcpConnection>> &record, detail::NativeSocket socket)
    {
      int error{};
      if (detail::connection_error(socket, error) != 0)
      {
        record->publish_native_for_test(vix::Completion<TcpConnection>{detail::connect_error(detail::last_socket_error(), "getsockopt")});
        return;
      }
      if (error != 0)
      {
        record->publish_native_for_test(vix::Completion<TcpConnection>{detail::connect_error(error, "connect")});
        return;
      }
      record->publish_native_success_with([](detail::NativeSocket connected_socket, detail::ProviderResourceLease lease)
                                          { return detail::make_tcp_connection(connected_socket, std::move(lease)); });
    }
  }
  vix::Result<Provider> Provider::create()
  {
    auto created = detail::ProviderCore::create();
    if (!created)
      return created.error();
    return Provider{std::static_pointer_cast<void>(created.take())};
  }
  vix::Result<Provider> Provider::create_cooperative()
  {
    auto created = detail::ProviderCore::create(true);
    if (!created)
      return created.error();
    return Provider{std::static_pointer_cast<void>(created.take())};
  }
  Provider::~Provider()
  {
    if (core_)
    {
      auto core = std::static_pointer_cast<detail::ProviderCore>(core_);
      if ((core->cooperative() && !core->on_owner_thread()) || core->has_dependencies())
        std::terminate();
    }
  }
  vix::Result<void> Provider::shutdown()
  {
    if (!core_)
      return vix::Error{provider_stopped, "provider has no lifecycle authority"};
    return std::static_pointer_cast<detail::ProviderCore>(core_)->shutdown();
  }
  vix::Result<std::size_t> Provider::poll()
  {
    if (!core_)
      return vix::Error{provider_stopped, "provider has no lifecycle authority"};
    return std::static_pointer_cast<detail::ProviderCore>(core_)->poll();
  }
  Operation<TcpConnection> Provider::connect(const Endpoint &endpoint, OperationOptions options)
  {
    auto core = std::static_pointer_cast<detail::ProviderCore>(core_);
    return detail::OperationFactory<TcpConnection>::make(core, [core, endpoint, options](auto record) mutable
                                                         {
      if (core && core->cooperative() && !core->on_owner_thread())
      { record->complete(vix::Completion<TcpConnection>{vix::Error{operation_in_progress, "cooperative provider is thread-affine"}}); return; }
      if (!core || !core->running()) { record->complete(vix::Completion<TcpConnection>{vix::Error{provider_stopped,"provider is stopped"}}); return; }
      if (!endpoint.port()) { record->complete(vix::Completion<TcpConnection>{vix::Error{invalid_port,"destination port must not be zero"}}); return; }
      record->observe_cancellation(options.cancellation);
      if (options.deadline) record->observe_deadline(*options.deadline);
      auto weak=record->weak_from_this();
      if (!core->post([weak,core,endpoint]
      {
        auto pending=weak.lock();
        if (!pending) return;
        const auto phase=pending->phase();
        if (phase!=detail::OperationRecord<TcpConnection>::Phase::suspending && phase!=detail::OperationRecord<TcpConnection>::Phase::suspended) return;
        const auto address=detail_native_endpoint::make(endpoint);
        auto created=detail::create_tcp_socket(endpoint.address().family());
        if (!created) { pending->publish_native_for_test(vix::Completion<TcpConnection>{created.error()}); return; }
        const auto socket=created.take();
        if (!pending->adopt_native_for_connect(socket)) { (void)detail::close_native_socket(socket); std::terminate(); }
        if (detail::connect_socket(socket,address)==0)
        {
          pending->publish_native_success_with([](detail::NativeSocket connected_socket, detail::ProviderResourceLease lease)
          { return detail::make_tcp_connection(connected_socket,std::move(lease)); });
          return;
        }
        const int error=detail::last_socket_error();
        if (!detail::socket_error_connect_pending(error)) { pending->publish_native_for_test(vix::Completion<TcpConnection>{detail::connect_error(error,"connect")}); return; }
        const auto token=core->register_descriptor(socket,{},[weak,socket](std::uint32_t)
        { if(auto locked_record=weak.lock()) publish_connect_result(locked_record,socket); return false; });
        if (!token) { pending->publish_native_for_test(vix::Completion<TcpConnection>{vix::Error{io_failure,"cannot register connect socket"}}); return; }
        pending->bind_descriptor_direction(token,false);
      })) record->complete(vix::Completion<TcpConnection>{vix::Error{provider_stopped,"provider is stopped"}}); });
  }
  vix::Result<TcpListener> Provider::listen(const Endpoint &endpoint, ListenOptions options)
  {
    auto core = std::static_pointer_cast<detail::ProviderCore>(core_);
    if (!core || !core->running())
      return vix::Error{provider_stopped, "provider is stopped"};
    if (core->cooperative() && !core->on_owner_thread())
      return vix::Error{operation_in_progress, "cooperative provider is thread-affine"};
    if (!options.backlog)
      return vix::Error{invalid_port, "listen backlog must not be zero"};
    if (options.backlog > static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
      return vix::Error{io_failure, "listen backlog exceeds native range"};
    const auto address = detail_native_endpoint::make(endpoint);
    auto created = detail::create_tcp_socket(endpoint.address().family());
    if (!created)
      return created.error();
    const auto socket = created.take();
    if (detail::bind_socket(socket, address) != 0)
    {
      const int error = detail::last_socket_error();
      (void)detail::close_native_socket(socket);
      if (detail::socket_error_address_in_use(error))
        return vix::Error{address_in_use, "bind"};
      return detail::connect_error(error, "bind");
    }
    if (detail::listen_socket(socket, static_cast<int>(options.backlog)) != 0)
    {
      const int error = detail::last_socket_error();
      (void)detail::close_native_socket(socket);
      return detail::connect_error(error, "listen");
    }
    auto local = detail::local_socket_address(socket);
    if (!local)
    {
      (void)detail::close_native_socket(socket);
      return local.error();
    }
    auto local_endpoint = detail::endpoint_from_native_address(local.value());
    if (!local_endpoint)
    {
      (void)detail::close_native_socket(socket);
      return local_endpoint.error();
    }
    return TcpListener{std::make_shared<detail::TcpListenerState>(socket, local_endpoint.take(), std::move(core))};
  }
  TcpConnection::TcpConnection(std::shared_ptr<detail::TcpConnectionState> state) noexcept : state_(std::move(state)) {}
  TcpConnection::TcpConnection(TcpConnection &&) noexcept = default;
  TcpConnection::~TcpConnection() = default;
  vix::Result<void> TcpConnection::close()
  {
    if (!state_)
      return vix::Result<void>::success();
    return state_->close();
  }
  vix::Result<void> TcpConnection::shutdown_write()
  {
    if (!state_)
      return vix::Error{peer_closed,"connection is closed"};
    return state_->shutdown_write();
  }
  Operation<std::size_t> TcpConnection::receive(std::span<std::byte> buffer, OperationOptions options)
  {
    if (!state_)
      std::terminate();
    return state_->receive(buffer, std::move(options));
  }
  Operation<void> TcpConnection::send(std::span<const std::byte> buffer, OperationOptions options)
  {
    if (!state_)
      std::terminate();
    return state_->send(buffer, std::move(options));
  }
  TcpListener::TcpListener(std::shared_ptr<detail::TcpListenerState> state) noexcept : state_(std::move(state)) {}
  TcpListener::TcpListener(TcpListener &&) noexcept = default;
  TcpListener::~TcpListener() = default;
  Operation<TcpConnection> TcpListener::accept(OperationOptions options)
  {
    if (!state_)
      std::terminate();
    return state_->accept(std::move(options));
  }
  vix::Result<Endpoint> TcpListener::local_endpoint() const
  {
    if (!state_)
      return vix::Error{peer_closed, "listener is closed"};
    return state_->local_endpoint();
  }
  vix::Result<void> TcpListener::close()
  {
    if (!state_)
      return vix::Result<void>::success();
    return state_->close();
  }
}

namespace vix::network::detail
{
  TcpConnection make_tcp_connection(NativeSocket fd, ProviderResourceLease lease)
  {
    return NetworkAccess::from_handoff(fd, std::move(lease));
  }
  TcpConnection NetworkAccess::adopt_tcp_for_test(Provider &provider, int fd)
  {
    return TcpConnection{std::make_shared<TcpConnectionState>(
        fd, std::static_pointer_cast<ProviderCore>(provider.core_))};
  }
  std::shared_ptr<ProviderCore> NetworkAccess::core_for_test(const Provider &provider) { return std::static_pointer_cast<ProviderCore>(provider.core_); }
  int NetworkAccess::native_fd_for_test(const TcpConnection &connection) noexcept { return connection.state_ ? connection.state_->native_fd_for_test() : -1; }
  void NetworkAccess::hold_transport_progression_for_test(const TcpConnection &connection, bool hold) noexcept
  {
    if (connection.state_)
      connection.state_->hold_progression_for_test(hold);
  }
  void NetworkAccess::fail_descriptor_arm_for_test(const TcpConnection &connection, bool fail) noexcept
  {
    if (connection.state_)
      connection.state_->fail_descriptor_arm_for_test(fail);
  }
  bool NetworkAccess::post_read_progression_for_test(const TcpConnection &connection)
  {
    return connection.state_ && connection.state_->post_read_progression_for_test();
  }
  bool NetworkAccess::post_write_progression_for_test(const TcpConnection &connection)
  {
    return connection.state_ && connection.state_->post_write_progression_for_test();
  }
  void NetworkAccess::publish_receive_for_test(const TcpConnection &connection, vix::Completion<std::size_t> completion)
  {
    if (connection.state_)
      connection.state_->publish_receive_for_test(std::move(completion));
  }
  void NetworkAccess::publish_send_for_test(const TcpConnection &connection, vix::Completion<void> completion)
  {
    if (connection.state_)
      connection.state_->publish_send_for_test(std::move(completion));
  }
  #ifdef VIX_NETWORK_TEST_CAPTURE
  void NetworkAccess::capture_next_receive_record_for_test(const TcpConnection &connection, std::shared_ptr<OperationRecord<std::size_t>> &record)
  {
    read_capture_connection_for_test = connection.state_.get();
    read_capture_for_test = [&record](std::shared_ptr<OperationRecord<std::size_t>> captured)
    { record = std::move(captured); };
  }
  void NetworkAccess::capture_next_send_record_for_test(const TcpConnection &connection, std::shared_ptr<OperationRecord<void>> &record)
  {
    write_capture_connection_for_test = connection.state_.get();
    write_capture_for_test = [&record](std::shared_ptr<OperationRecord<void>> captured)
    { record = std::move(captured); };
  }
  #endif
  NetworkAccess::TransportCountersForTest NetworkAccess::transport_counters_for_test(const TcpConnection &connection)
  {
    return connection.state_ ? connection.state_->transport_counters_for_test() : TransportCountersForTest{};
  }
  TcpConnection NetworkAccess::from_handoff(NativeSocket fd, ProviderResourceLease lease)
  {
    return TcpConnection{std::make_shared<TcpConnectionState>(fd, std::move(lease))};
  }
}
