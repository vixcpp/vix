#pragma once

#include "Errors.hpp"

#include <vix/Result.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <mutex>
#include <span>
#include <string_view>

typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st SSL;
typedef struct bio_st BIO;

namespace vix::tls::detail
{
  enum class EngineWant
  {
    complete,
    retry,
    read,
    write,
    closed,
    failure
  };

  struct EngineProgress
  {
    EngineWant want{EngineWant::failure};
    std::size_t bytes{};
    vix::ErrorIdentity error{backend_failure};
    std::size_t ciphertext_bytes{};
  };

  // Private OpenSSL 3 session driver. The network BIO is the only ciphertext
  // boundary exposed to the parent operation; SSL/BIO state is always entered
  // under mutex_, and no lock is held while Network work is awaited.
  class OpenSslDriver final
  {
  public:
    static constexpr std::size_t buffer_capacity = 32U * 1024U;

    struct ShutdownDiagnostics
    {
      int ssl_shutdown_flags{};
      int last_ssl_result{};
      int last_ssl_error{};
      std::size_t last_ciphertext_bytes{};
      std::size_t bio_pending_bytes{};
      bool started{};
      bool complete{};
      bool pending{};
      EngineWant pending_want{EngineWant::complete};
    };

    static vix::Result<std::unique_ptr<OpenSslDriver>> create(
        SSL_CTX *context, bool server_role,
        std::size_t bio_capacity = buffer_capacity,
        std::string_view expected_identity = {}, bool identity_is_ip = false,
        std::string_view server_name = {});

    OpenSslDriver(const OpenSslDriver &) = delete;
    OpenSslDriver &operator=(const OpenSslDriver &) = delete;
    ~OpenSslDriver();

    EngineProgress handshake_step() noexcept;
    EngineProgress handshake_step(std::span<std::byte> ciphertext) noexcept;
    EngineProgress shutdown_step(std::span<std::byte> ciphertext) noexcept;
    EngineProgress retry_shutdown(std::span<std::byte> ciphertext) noexcept;
    EngineProgress write_step(std::span<const std::byte> input) noexcept;
    EngineProgress write_step(std::span<const std::byte> input,
                              std::span<std::byte> ciphertext) noexcept;
    EngineProgress retry_write() noexcept;
    EngineProgress retry_write(std::span<std::byte> ciphertext) noexcept;
    EngineProgress read_step() noexcept;
    EngineProgress read_step(std::span<std::byte> ciphertext) noexcept;
    EngineProgress retry_read() noexcept;
    EngineProgress retry_read(std::span<std::byte> ciphertext) noexcept;

    [[nodiscard]] std::size_t pending_ciphertext() const noexcept;
    vix::Result<std::size_t> drain_ciphertext(std::span<std::byte> output) noexcept;
    vix::Result<std::size_t> feed_ciphertext(std::span<const std::byte> input) noexcept;
    [[nodiscard]] bool plaintext_available() const noexcept;
    vix::Result<std::size_t> take_plaintext(std::span<std::byte> output) noexcept;
    [[nodiscard]] ShutdownDiagnostics shutdown_diagnostics_for_test() const noexcept;

  private:
    enum class PendingCall { handshake, shutdown, read, write };
    struct PendingDirection
    {
      bool active{};
      EngineWant want{EngineWant::complete};
    };
    OpenSslDriver(SSL_CTX *context, SSL *session, BIO *network_bio) noexcept;
    EngineProgress invoke(PendingCall call, std::span<std::byte> ciphertext) noexcept;

    mutable std::mutex mutex_;
    SSL_CTX *context_{};
    SSL *session_{};
    BIO *network_bio_{};
    bool handshake_pending_{};
    PendingDirection read_pending_{};
    PendingDirection write_pending_{};
    bool handshake_complete_{};
    bool shutdown_started_{};
    bool shutdown_complete_{};
    PendingDirection shutdown_pending_{};
    int last_shutdown_result_{};
    int last_shutdown_error_{};
    std::size_t last_shutdown_ciphertext_{};
    bool read_result_ready_{};
    std::size_t write_size_{};
    std::size_t read_size_{};
    std::array<std::byte, buffer_capacity> write_buffer_{};
    std::array<std::byte, buffer_capacity> read_buffer_{};
  };
}
