#include "OpenSslDriver.hpp"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <algorithm>
#include <climits>
#include <new>

namespace vix::tls::detail
{
  namespace
  {
    EngineProgress failure(vix::ErrorIdentity identity = backend_failure) noexcept
    {
      return {EngineWant::failure, 0, identity};
    }
  }

  OpenSslDriver::OpenSslDriver(SSL_CTX *context, SSL *session, BIO *network_bio) noexcept
      : context_(context), session_(session), network_bio_(network_bio)
  {
  }

  vix::Result<std::unique_ptr<OpenSslDriver>> OpenSslDriver::create(
      SSL_CTX *context, bool server_role, std::size_t bio_capacity,
      std::string_view expected_identity, bool identity_is_ip,
      std::string_view server_name)
  {
    if (bio_capacity == 0 || bio_capacity > buffer_capacity)
      return vix::Error{buffer_limit, "OpenSSL BIO capacity is outside the private bound"};
    if (context == nullptr || SSL_CTX_up_ref(context) != 1)
      return vix::Error{backend_failure, "OpenSSL context is unavailable"};
    std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> context_ref{
        context, SSL_CTX_free};

    std::unique_ptr<SSL, decltype(&SSL_free)> session{SSL_new(context), SSL_free};
    if (!session)
      return vix::Error{backend_failure, "OpenSSL session allocation failed"};

    struct BioDeleter
    {
      void operator()(BIO *bio) const noexcept
      {
        if (bio != nullptr)
          BIO_free(bio);
      }
    };
    std::unique_ptr<BIO, BioDeleter> ssl_bio;
    std::unique_ptr<BIO, BioDeleter> network_bio;
    BIO *ssl_bio_raw{};
    BIO *network_bio_raw{};
    if (BIO_new_bio_pair(&ssl_bio_raw, bio_capacity,
                         &network_bio_raw, bio_capacity) != 1)
    {
      ssl_bio.reset(ssl_bio_raw);
      network_bio.reset(network_bio_raw);
      return vix::Error{backend_failure, "OpenSSL bounded BIO pair allocation failed"};
    }
    ssl_bio.reset(ssl_bio_raw);
    network_bio.reset(network_bio_raw);

    // SSL owns the engine side. The driver exclusively owns the network side.
    SSL_set_bio(session.get(), ssl_bio.get(), ssl_bio.get());
    (void)ssl_bio.release();
    if (server_role)
      SSL_set_accept_state(session.get());
    else
    {
      if (!expected_identity.empty())
      {
        X509_VERIFY_PARAM *parameters = SSL_get0_param(session.get());
        const int configured = identity_is_ip
            ? X509_VERIFY_PARAM_set1_ip_asc(parameters,
                                            std::string{expected_identity}.c_str())
            : X509_VERIFY_PARAM_set1_host(parameters,
                                          expected_identity.data(),
                                          expected_identity.size());
        if (configured != 1)
          return vix::Error{invalid_configuration,
                            "TLS peer identity could not be configured"};
      }
      if (!server_name.empty() &&
          SSL_set_tlsext_host_name(session.get(), std::string{server_name}.c_str()) != 1)
        return vix::Error{invalid_configuration, "TLS SNI could not be configured"};
      SSL_set_connect_state(session.get());
    }

    auto *driver = new (std::nothrow) OpenSslDriver(
        context_ref.release(), session.release(), network_bio.release());
    if (driver == nullptr)
      return vix::Error{backend_failure, "TLS driver allocation failed"};
    return std::unique_ptr<OpenSslDriver>{driver};
  }

  OpenSslDriver::~OpenSslDriver()
  {
    BIO_free(network_bio_);
    SSL_free(session_);
    SSL_CTX_free(context_);
  }

  EngineProgress OpenSslDriver::invoke(PendingCall call,
                                      std::span<std::byte> ciphertext) noexcept
  {
    const bool retrying_shutdown_zero_return =
        call == PendingCall::shutdown && shutdown_pending_.active &&
        shutdown_pending_.want == EngineWant::retry;
    if (call == PendingCall::handshake)
    {
      if (read_pending_.active || write_pending_.active || shutdown_started_)
        return failure(operation_conflict);
      handshake_pending_ = true;
    }
    else if (call == PendingCall::shutdown)
    {
      if (!handshake_complete_ || read_pending_.active || write_pending_.active)
        return failure(operation_conflict);
      shutdown_started_ = true;
    }
    else if (handshake_pending_)
      return failure(operation_conflict);
    else if (shutdown_started_)
      return failure(channel_closed);
    // OpenSSL permits a read to process inbound data while a write is waiting
    // for WANT_READ (and vice versa). A WANT_WRITE retry is different: no other
    // TLS I/O may run until that exact operation is retried.
    else if (call == PendingCall::read &&
             write_pending_.active && write_pending_.want == EngineWant::write)
      return failure(operation_conflict);
    else if (call == PendingCall::write &&
             read_pending_.active && read_pending_.want == EngineWant::write)
      return failure(operation_conflict);

    ERR_clear_error();
    int result{};
    std::size_t transferred{};
    switch (call)
    {
    case PendingCall::handshake:
      result = SSL_do_handshake(session_);
      break;
    case PendingCall::shutdown:
      result = SSL_shutdown(session_);
      break;
    case PendingCall::read:
      result = SSL_read_ex(session_, read_buffer_.data(), read_buffer_.size(), &transferred);
      break;
    case PendingCall::write:
      result = SSL_write_ex(session_, write_buffer_.data(), write_size_, &transferred);
      break;
    }

    // OpenSSL requires this immediately after the corresponding SSL call and
    // on the same thread; keep it inside this serialized critical section.
    // SSL_{read,write}_ex report failure with 0 (not a negative value), while
    // SSL_shutdown's 0 is its normal "close_notify sent, await peer" state.
    // Only successful operations and that special shutdown state may skip
    // SSL_get_error; all other results must be classified immediately.
    const bool shutdown_wait = call == PendingCall::shutdown && result == 0;
    const int ssl_error = result == 1 || shutdown_wait
                              ? SSL_ERROR_NONE
                              : SSL_get_error(session_, result);
    std::size_t ciphertext_size{};
    if (!ciphertext.empty())
    {
      const auto pending = static_cast<std::size_t>(BIO_ctrl_pending(network_bio_));
      if (pending > ciphertext.size())
        return failure(buffer_limit);
      if (pending != 0)
      {
        const int count = BIO_read(network_bio_, ciphertext.data(),
                                  static_cast<int>(pending));
        if (count <= 0)
          return failure(backend_failure);
        ciphertext_size = static_cast<std::size_t>(count);
      }
    }
    if (call == PendingCall::shutdown)
    {
      last_shutdown_result_ = result;
      last_shutdown_error_ = ssl_error;
      last_shutdown_ciphertext_ = ciphertext_size;
    }
    if (call == PendingCall::shutdown && result == 0)
    {
      shutdown_pending_ = {true, EngineWant::read};
      return {EngineWant::read, 0, backend_failure, ciphertext_size};
    }
    if (result == 1)
    {
      if (call == PendingCall::handshake)
      {
        if ((SSL_get_verify_mode(session_) & SSL_VERIFY_PEER) != 0 &&
            SSL_get_verify_result(session_) != X509_V_OK)
        {
          handshake_pending_ = false;
          return {EngineWant::failure, 0, authentication_failed, ciphertext_size};
        }
        handshake_complete_ = true;
        handshake_pending_ = false;
        return {EngineWant::complete, 0, backend_failure, ciphertext_size};
      }
      if (call == PendingCall::read)
      {
        read_pending_ = {};
        read_size_ = transferred;
        read_result_ready_ = true;
      }
      else if (call == PendingCall::shutdown)
      {
        shutdown_pending_ = {};
        shutdown_complete_ = true;
        shutdown_started_ = false;
        return {EngineWant::complete, 0, backend_failure, ciphertext_size};
      }
      else
        write_pending_ = {};
      return {EngineWant::complete, transferred, backend_failure, ciphertext_size};
    }
    if (ssl_error == SSL_ERROR_WANT_READ)
    {
      if (call == PendingCall::read)
        read_pending_ = {true, EngineWant::read};
      else if (call == PendingCall::write)
        write_pending_ = {true, EngineWant::read};
      else if (call == PendingCall::shutdown)
        shutdown_pending_ = {true, EngineWant::read};
      return {EngineWant::read, 0, backend_failure, ciphertext_size};
    }
    if (ssl_error == SSL_ERROR_WANT_WRITE)
    {
      if (call == PendingCall::read)
        read_pending_ = {true, EngineWant::write};
      else if (call == PendingCall::write)
        write_pending_ = {true, EngineWant::write};
      else if (call == PendingCall::shutdown)
        shutdown_pending_ = {true, EngineWant::write};
      return {EngineWant::write, 0, backend_failure, ciphertext_size};
    }
    if (ssl_error == SSL_ERROR_ZERO_RETURN)
    {
      if (call == PendingCall::shutdown)
      {
        const int shutdown_state = SSL_get_shutdown(session_);
        if ((shutdown_state & SSL_SENT_SHUTDOWN) == 0 &&
            !retrying_shutdown_zero_return)
        {
          // Peer close_notify does not prove that our own alert was sent.
          shutdown_pending_ = {true, EngineWant::retry};
          return {EngineWant::retry, 0, backend_failure, ciphertext_size};
        }
        shutdown_pending_ = {};
        shutdown_started_ = false;
        return failure(protocol_failure);
      }
      if (call == PendingCall::read)
        read_pending_ = {};
      else if (call == PendingCall::write)
        write_pending_ = {};
      else if (call == PendingCall::shutdown)
      {
        shutdown_pending_ = {};
        shutdown_complete_ = true;
        shutdown_started_ = false;
      }
      return {EngineWant::closed, 0, peer_closed, ciphertext_size};
    }
    if (call == PendingCall::read)
      read_pending_ = {};
    else if (call == PendingCall::write)
      write_pending_ = {};
    else if (call == PendingCall::shutdown)
      shutdown_pending_ = {};
    if (call == PendingCall::handshake)
    {
      const auto verify_result = SSL_get_verify_result(session_);
      if (verify_result == X509_V_ERR_HOSTNAME_MISMATCH ||
          verify_result == X509_V_ERR_IP_ADDRESS_MISMATCH)
        return {EngineWant::failure, 0, peer_identity_mismatch, ciphertext_size};
      if (verify_result != X509_V_OK)
        return {EngineWant::failure, 0, authentication_failed, ciphertext_size};
    }
    return {EngineWant::failure, 0,
            ssl_error == SSL_ERROR_SSL ? protocol_failure : backend_failure,
            ciphertext_size};
  }

  EngineProgress OpenSslDriver::handshake_step() noexcept
  {
    std::lock_guard lock(mutex_);
    if (handshake_complete_)
      return {EngineWant::complete, 0, backend_failure};
    return invoke(PendingCall::handshake, {});
  }

  EngineProgress OpenSslDriver::handshake_step(std::span<std::byte> ciphertext) noexcept
  {
    std::lock_guard lock(mutex_);
    if (handshake_complete_)
      return {EngineWant::complete, 0, backend_failure};
    return invoke(PendingCall::handshake, ciphertext);
  }

  EngineProgress OpenSslDriver::shutdown_step(
      std::span<std::byte> ciphertext) noexcept
  {
    std::lock_guard lock(mutex_);
    if (shutdown_complete_)
      return {EngineWant::complete, 0, backend_failure};
    return invoke(PendingCall::shutdown, ciphertext);
  }

  EngineProgress OpenSslDriver::retry_shutdown(
      std::span<std::byte> ciphertext) noexcept
  {
    std::lock_guard lock(mutex_);
    if (!shutdown_pending_.active)
      return failure(operation_conflict);
    return invoke(PendingCall::shutdown, ciphertext);
  }

  EngineProgress OpenSslDriver::write_step(std::span<const std::byte> input) noexcept
  {
    std::lock_guard lock(mutex_);
    if (!handshake_complete_)
      return failure(operation_conflict);
    if (!write_pending_.active)
    {
      if (input.empty() || input.size() > write_buffer_.size())
        return failure(buffer_limit);
      std::copy(input.begin(), input.end(), write_buffer_.begin());
      write_size_ = input.size();
    }
    else if (input.size() != write_size_ ||
             !std::equal(input.begin(), input.end(), write_buffer_.begin()))
      return failure(operation_conflict);
    return invoke(PendingCall::write, {});
  }

  EngineProgress OpenSslDriver::write_step(std::span<const std::byte> input,
                                           std::span<std::byte> ciphertext) noexcept
  {
    std::lock_guard lock(mutex_);
    if (!handshake_complete_)
      return failure(operation_conflict);
    if (!write_pending_.active)
    {
      if (input.empty() || input.size() > write_buffer_.size())
        return failure(buffer_limit);
      std::copy(input.begin(), input.end(), write_buffer_.begin());
      write_size_ = input.size();
    }
    else if (input.size() != write_size_ ||
             !std::equal(input.begin(), input.end(), write_buffer_.begin()))
      return failure(operation_conflict);
    return invoke(PendingCall::write, ciphertext);
  }

  EngineProgress OpenSslDriver::retry_write() noexcept
  {
    std::lock_guard lock(mutex_);
    if (!write_pending_.active)
      return failure(operation_conflict);
    return invoke(PendingCall::write, {});
  }

  EngineProgress OpenSslDriver::retry_write(std::span<std::byte> ciphertext) noexcept
  {
    std::lock_guard lock(mutex_);
    if (!write_pending_.active)
      return failure(operation_conflict);
    return invoke(PendingCall::write, ciphertext);
  }

  EngineProgress OpenSslDriver::read_step() noexcept
  {
    std::lock_guard lock(mutex_);
    if (!handshake_complete_)
      return failure(operation_conflict);
    if (read_result_ready_)
      return {EngineWant::complete, read_size_, backend_failure};
    return invoke(PendingCall::read, {});
  }

  EngineProgress OpenSslDriver::read_step(std::span<std::byte> ciphertext) noexcept
  {
    std::lock_guard lock(mutex_);
    if (!handshake_complete_)
      return failure(operation_conflict);
    if (read_result_ready_)
      return {EngineWant::complete, read_size_, backend_failure};
    return invoke(PendingCall::read, ciphertext);
  }

  EngineProgress OpenSslDriver::retry_read() noexcept
  {
    std::lock_guard lock(mutex_);
    if (!read_pending_.active)
      return failure(operation_conflict);
    return invoke(PendingCall::read, {});
  }

  EngineProgress OpenSslDriver::retry_read(std::span<std::byte> ciphertext) noexcept
  {
    std::lock_guard lock(mutex_);
    if (!read_pending_.active)
      return failure(operation_conflict);
    return invoke(PendingCall::read, ciphertext);
  }

  std::size_t OpenSslDriver::pending_ciphertext() const noexcept
  {
    std::lock_guard lock(mutex_);
    return static_cast<std::size_t>(BIO_ctrl_pending(network_bio_));
  }

  OpenSslDriver::ShutdownDiagnostics
  OpenSslDriver::shutdown_diagnostics_for_test() const noexcept
  {
    std::lock_guard lock(mutex_);
    return {SSL_get_shutdown(session_), last_shutdown_result_, last_shutdown_error_,
            last_shutdown_ciphertext_,
            static_cast<std::size_t>(BIO_ctrl_pending(network_bio_)),
            shutdown_started_, shutdown_complete_, shutdown_pending_.active,
            shutdown_pending_.want};
  }

  vix::Result<std::size_t> OpenSslDriver::drain_ciphertext(
      std::span<std::byte> output) noexcept
  {
    std::lock_guard lock(mutex_);
    if (output.empty())
      return std::size_t{0};
    if (BIO_ctrl_pending(network_bio_) == 0)
      return std::size_t{0};
    const auto limit = std::min(output.size(), static_cast<std::size_t>(INT_MAX));
    const int count = BIO_read(network_bio_, output.data(), static_cast<int>(limit));
    if (count < 0)
      return vix::Error{backend_failure, "OpenSSL outbound BIO read failed"};
    return static_cast<std::size_t>(count);
  }

  vix::Result<std::size_t> OpenSslDriver::feed_ciphertext(
      std::span<const std::byte> input) noexcept
  {
    std::lock_guard lock(mutex_);
    std::size_t offset{};
    while (offset < input.size())
    {
      const auto remaining = input.size() - offset;
      const auto limit = std::min(remaining, static_cast<std::size_t>(INT_MAX));
      const int count = BIO_write(network_bio_, input.data() + offset,
                                  static_cast<int>(limit));
      if (count <= 0)
        break;
      offset += static_cast<std::size_t>(count);
    }
    return offset;
  }

  bool OpenSslDriver::plaintext_available() const noexcept
  {
    std::lock_guard lock(mutex_);
    return read_result_ready_;
  }

  vix::Result<std::size_t> OpenSslDriver::take_plaintext(
      std::span<std::byte> output) noexcept
  {
    std::lock_guard lock(mutex_);
    if (!read_result_ready_)
      return vix::Error{operation_conflict, "plaintext result is unavailable"};
    const auto size = std::min(output.size(), read_size_);
    std::copy_n(read_buffer_.begin(), size, output.begin());
    if (size < read_size_)
      std::move(read_buffer_.begin() + static_cast<std::ptrdiff_t>(size),
                read_buffer_.begin() + static_cast<std::ptrdiff_t>(read_size_),
                read_buffer_.begin());
    read_size_ -= size;
    read_result_ready_ = read_size_ != 0;
    return size;
  }
}
