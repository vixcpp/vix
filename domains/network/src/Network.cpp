#include "Engine.hpp"
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Errors.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <optional>
#include <unistd.h>
#include <utility>

namespace vix::network::detail
{
  namespace
  {
    vix::Error transport_error(int error, const char *where)
    {
      const auto identity = [](int value)
      {
        switch (value)
        {
        case ECONNRESET:
          return connection_reset;
        case ECONNABORTED:
          return connection_aborted;
        case EPIPE:
        case ENOTCONN:
          return peer_closed;
        case ENETUNREACH:
          return network_unreachable;
        case EHOSTUNREACH:
          return host_unreachable;
        case ETIMEDOUT:
          return timed_out;
        case EACCES:
        case EPERM:
          return permission_denied;
        case ENOMEM:
        case ENOBUFS:
          return resource_exhausted;
        default:
          return io_failure;
        }
      }(error);
      return vix::Error{identity, where};
    }
  }
  vix::Error transport_error_for_test(int error) noexcept { return transport_error(error,"test"); }
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
    TcpConnectionState(int fd, std::shared_ptr<ProviderCore> provider) noexcept : fd_(fd), provider_(provider), lease_(std::in_place, std::move(provider)) {}
    TcpConnectionState(int fd, ProviderResourceLease lease) noexcept : fd_(fd), provider_(lease.provider()), lease_(std::in_place, std::move(lease)) {}
    ~TcpConnectionState() { (void)close(); }
    vix::Result<void> close() noexcept
    {
      std::lock_guard lock(mutex_);
      if (closed_)
        return error_ ? vix::Result<void>{*error_} : vix::Result<void>::success();
      if (read_active_ || write_active_)
        return vix::Error{operation_in_progress, "transport operation is active"};
      closed_ = true;
      if (descriptor_)
        provider_->retire(std::exchange(descriptor_, 0));
      const int fd = std::exchange(fd_, -1);
      if (fd >= 0 && ::close(fd) != 0)
        error_.emplace(vix::Error{close_failed, "close"});
      lease_.reset();
      return error_ ? vix::Result<void>{*error_} : vix::Result<void>::success();
    }
    vix::Result<void> shutdown_write() noexcept
    {
      std::lock_guard lock(mutex_);
      if (closed_)
        return vix::Error{peer_closed,"connection is closed"};
      if (write_active_)
        return vix::Error{operation_in_progress,"write operation is active"};
      if (write_shutdown_)
        return vix::Result<void>::success();
      for (;;)
      {
        if (::shutdown(fd_,SHUT_WR)==0)
        {
          write_shutdown_=true;
          return vix::Result<void>::success();
        }
        if (errno==EINTR)
          continue;
        return transport_error(errno,"shutdown write");
      }
    }
    int native_fd_for_test() const noexcept { return fd_; }
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
    int fd_{-1};
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
    int fd;
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
      const auto result = ::recv(fd, buffer.data(), buffer.size(), 0);
      if (result >= 0)
      {
        record->publish_native_for_test(vix::Completion<std::size_t>{static_cast<std::size_t>(result)});
        return;
      }
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;
      record->publish_native_for_test(vix::Completion<std::size_t>{transport_error(errno, "recv")});
      return;
    }
  }
  void TcpConnectionState::progress_send()
  {
    std::shared_ptr<OperationRecord<void>> record;
    std::span<const std::byte> buffer;
    std::size_t offset;
    int fd;
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
      const auto result = ::send(fd, buffer.data() + offset, buffer.size() - offset, MSG_NOSIGNAL);
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
      if (result < 0 && errno == EINTR)
        continue;
      if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return;
      record->publish_native_for_test(vix::Completion<void>{transport_error(result < 0 ? errno : EPIPE, "send")});
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
}

namespace vix::network
{
  struct detail_native_endpoint
  {
    static sockaddr_storage make(const Endpoint &endpoint, socklen_t &size) noexcept
    {
      sockaddr_storage storage{};
      if (endpoint.address_.family_ == AddressFamily::ipv4)
      {
        auto &address = reinterpret_cast<sockaddr_in &>(storage);
        address.sin_family = AF_INET;
        address.sin_port = htons(endpoint.port_);
        std::memcpy(&address.sin_addr, endpoint.address_.bytes_.data(), 4);
        size = sizeof address;
      }
      else
      {
        auto &address = reinterpret_cast<sockaddr_in6 &>(storage);
        address.sin6_family = AF_INET6;
        address.sin6_port = htons(endpoint.port_);
        address.sin6_scope_id = endpoint.address_.scope_id_;
        std::memcpy(&address.sin6_addr, endpoint.address_.bytes_.data(), 16);
        size = sizeof address;
      }
      return storage;
    }
  };
}

namespace vix::network
{
  namespace
  {
    vix::Error connect_error(int error, const char *where)
    {
      const auto identity = [](int value)
      {
        switch (value)
        {
        case ECONNREFUSED:
          return connection_refused;
        case ECONNRESET:
          return connection_reset;
        case ECONNABORTED:
          return connection_aborted;
        case ENETUNREACH:
          return network_unreachable;
        case EHOSTUNREACH:
          return host_unreachable;
        case ETIMEDOUT:
          return timed_out;
        case EADDRNOTAVAIL:
          return address_unavailable;
        case EACCES:
        case EPERM:
          return permission_denied;
        case EMFILE:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
          return resource_exhausted;
        case EAFNOSUPPORT:
        case EPROTONOSUPPORT:
        case ESOCKTNOSUPPORT:
          return unsupported;
        default:
          return io_failure;
        }
      }(error);
      return vix::Error{identity, where};
    }
    void publish_connect_result(const std::shared_ptr<detail::OperationRecord<TcpConnection>> &record, int fd)
    {
      int error{};
      socklen_t size = sizeof error;
      if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size) != 0)
      {
        record->publish_native_for_test(vix::Completion<TcpConnection>{connect_error(errno, "getsockopt")});
        return;
      }
      if (error != 0)
      {
        record->publish_native_for_test(vix::Completion<TcpConnection>{connect_error(error, "connect")});
        return;
      }
      record->publish_native_success_with([](int connected_fd, detail::ProviderResourceLease lease)
                                          { return detail::make_tcp_connection(connected_fd, std::move(lease)); });
    }
  }
  vix::Result<Provider> Provider::create()
  {
    auto created = detail::ProviderCore::create();
    if (!created)
      return created.error();
    return Provider{std::static_pointer_cast<void>(created.take())};
  }
  Provider::~Provider()
  {
    if (core_ && std::static_pointer_cast<detail::ProviderCore>(core_)->has_dependencies())
      std::terminate();
  }
  vix::Result<void> Provider::shutdown()
  {
    if (!core_)
      return vix::Error{provider_stopped, "provider has no lifecycle authority"};
    return std::static_pointer_cast<detail::ProviderCore>(core_)->shutdown();
  }
  Operation<TcpConnection> Provider::connect(const Endpoint &endpoint, OperationOptions options)
  {
    auto core = std::static_pointer_cast<detail::ProviderCore>(core_);
    return detail::OperationFactory<TcpConnection>::make(core, [core, endpoint, options](auto record) mutable
                                                         {
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
        socklen_t address_size{}; auto address=detail_native_endpoint::make(endpoint,address_size);
        const int family=reinterpret_cast<const sockaddr &>(address).sa_family;
        const int fd=::socket(family,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
        if (fd < 0) { pending->publish_native_for_test(vix::Completion<TcpConnection>{connect_error(errno,"socket")}); return; }
        if (!pending->adopt_native_for_connect(fd)) { (void)::close(fd); std::terminate(); }
        if (::connect(fd,reinterpret_cast<const sockaddr *>(&address),address_size)==0)
        {
          pending->publish_native_success_with([](int connected_fd, detail::ProviderResourceLease lease)
          { return detail::make_tcp_connection(connected_fd,std::move(lease)); });
          return;
        }
        const int error=errno;
        if (error != EINPROGRESS) { pending->publish_native_for_test(vix::Completion<TcpConnection>{connect_error(error,"connect")}); return; }
        const auto token=core->register_descriptor(fd,{},[weak,fd](std::uint32_t)
        { if(auto record=weak.lock()) publish_connect_result(record,fd); return false; });
        if (!token) { pending->publish_native_for_test(vix::Completion<TcpConnection>{vix::Error{io_failure,"cannot register connect socket"}}); return; }
        pending->bind_descriptor_direction(token,false);
      })) record->complete(vix::Completion<TcpConnection>{vix::Error{provider_stopped,"provider is stopped"}}); });
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
}

namespace vix::network::detail
{
  TcpConnection make_tcp_connection(int fd, ProviderResourceLease lease)
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
  TcpConnection NetworkAccess::from_handoff(int fd, ProviderResourceLease lease)
  {
    return TcpConnection{std::make_shared<TcpConnectionState>(fd, std::move(lease))};
  }
}
