#include "OpenSslDriver.hpp"
#include "PrivateChannel.hpp"

#include <vix/tls/TlsChannel.hpp>
#include <vix/tls/TlsConfig.hpp>

#include <vix/Cancellation.hpp>
#include <vix/Completion.hpp>
#include <vix/Deadline.hpp>
#include <vix/Duration.hpp>
#include <vix/network/Provider.hpp>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>

#include <sys/socket.h>

#include <array>
#include <algorithm>
#include <cassert>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <cstring>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <utility>

using namespace vix::tls::detail;
using namespace vix::network;
using namespace vix::network::detail;

namespace
{
  struct Contexts
  {
    SSL_CTX *client{SSL_CTX_new(TLS_client_method())};
    SSL_CTX *server{SSL_CTX_new(TLS_server_method())};
    EVP_PKEY *key{};
    X509 *certificate{};

    Contexts()
    {
      assert(client != nullptr && server != nullptr);
      SSL_CTX_set_verify(client, SSL_VERIFY_NONE, nullptr); // Feasibility fixture only.
      EVP_PKEY_CTX *key_context = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
      assert(key_context != nullptr);
      assert(EVP_PKEY_keygen_init(key_context) == 1);
      assert(EVP_PKEY_CTX_set_rsa_keygen_bits(key_context, 2048) == 1);
      assert(EVP_PKEY_keygen(key_context, &key) == 1);
      EVP_PKEY_CTX_free(key_context);

      certificate = X509_new();
      assert(certificate != nullptr);
      assert(X509_set_version(certificate, 2) == 1);
      assert(ASN1_INTEGER_set(X509_get_serialNumber(certificate), 1) == 1);
      assert(X509_gmtime_adj(X509_getm_notBefore(certificate), -60) != nullptr);
      assert(X509_gmtime_adj(X509_getm_notAfter(certificate), 3600) != nullptr);
      assert(X509_set_pubkey(certificate, key) == 1);
      X509_NAME *name = X509_get_subject_name(certificate);
      constexpr unsigned char common_name[] = "vix-test.invalid";
      assert(X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, common_name,
                                        -1, -1, 0) == 1);
      assert(X509_set_issuer_name(certificate, name) == 1);
      X509V3_CTX extension_context{};
      X509V3_set_ctx(&extension_context, certificate, certificate, nullptr, nullptr, 0);
      char basic_constraints_text[] = "critical,CA:TRUE";
      X509_EXTENSION *basic_constraints = X509V3_EXT_conf_nid(
          nullptr, &extension_context, NID_basic_constraints,
          basic_constraints_text);
      assert(basic_constraints != nullptr);
      assert(X509_add_ext(certificate, basic_constraints, -1) == 1);
      X509_EXTENSION_free(basic_constraints);
      char subject_alt_name_text[] = "DNS:vix-test.invalid,IP:127.0.0.1";
      X509_EXTENSION *subject_alt_name = X509V3_EXT_conf_nid(
          nullptr, &extension_context, NID_subject_alt_name,
          subject_alt_name_text);
      assert(subject_alt_name != nullptr);
      assert(X509_add_ext(certificate, subject_alt_name, -1) == 1);
      X509_EXTENSION_free(subject_alt_name);
      assert(X509_sign(certificate, key, EVP_sha256()) > 0);
      assert(SSL_CTX_use_certificate(server, certificate) == 1);
      assert(SSL_CTX_use_PrivateKey(server, key) == 1);
      assert(SSL_CTX_check_private_key(server) == 1);
    }

    ~Contexts()
    {
      X509_free(certificate);
      EVP_PKEY_free(key);
      SSL_CTX_free(server);
      SSL_CTX_free(client);
    }
  };

  struct PemCredentials
  {
    std::string certificate_path;
    std::string key_path;

    explicit PemCredentials(const Contexts &contexts)
    {
      char certificate_template[] = "/tmp/vix-tls-cert-XXXXXX";
      const int certificate_fd = ::mkstemp(certificate_template);
      assert(certificate_fd >= 0);
      certificate_path = certificate_template;
      FILE *certificate_file = ::fdopen(certificate_fd, "w");
      assert(certificate_file != nullptr);
      assert(PEM_write_X509(certificate_file, contexts.certificate) == 1);
      assert(std::fclose(certificate_file) == 0);

      char key_template[] = "/tmp/vix-tls-key-XXXXXX";
      const int key_fd = ::mkstemp(key_template);
      assert(key_fd >= 0);
      key_path = key_template;
      FILE *key_file = ::fdopen(key_fd, "w");
      assert(key_file != nullptr);
      assert(PEM_write_PrivateKey(key_file, contexts.key, nullptr, nullptr, 0,
                                  nullptr, nullptr) == 1);
      assert(std::fclose(key_file) == 0);
    }

    ~PemCredentials()
    {
      if (!certificate_path.empty())
        (void)::unlink(certificate_path.c_str());
      if (!key_path.empty())
        (void)::unlink(key_path.c_str());
    }
  };

  struct BoundedPipe
  {
    std::array<std::byte, 64U * 1024U> pending{};
    std::size_t begin{};
    std::size_t end{};
    std::size_t maximum_chunk{11};

    void progress(OpenSslDriver &from, OpenSslDriver &to)
    {
      if (begin != 0 && begin == end)
        begin = end = 0;
      if (begin != 0 && end == pending.size())
      {
        const auto remaining = end - begin;
        std::memmove(pending.data(), pending.data() + begin, remaining);
        begin = 0;
        end = remaining;
      }
      if (end < pending.size())
      {
        const auto available = std::min(maximum_chunk, pending.size() - end);
        auto drained = from.drain_ciphertext(
            std::span<std::byte>{pending.data() + end, available});
        assert(drained);
        end += drained.value();
      }
      if (begin < end)
      {
        const auto available = std::min(maximum_chunk, end - begin);
        auto fed = to.feed_ciphertext(
            std::span<const std::byte>{pending.data() + begin, available});
        assert(fed);
        begin += fed.value();
      }
    }
  };

  struct Observation
  {
    std::mutex mutex;
    std::condition_variable changed;
    std::optional<vix::Completion<void>> completion;

    bool done()
    {
      std::lock_guard lock(mutex);
      return completion.has_value();
    }

    bool wait_for(std::chrono::milliseconds timeout)
    {
      std::unique_lock lock(mutex);
      return changed.wait_for(lock, timeout, [&] { return completion.has_value(); });
    }

    vix::CompletionStatus status()
    {
      std::lock_guard lock(mutex);
      assert(completion);
      return completion->status();
    }
  };

  void report_handshake_failure(const char *side, Observation &observation)
  {
    std::lock_guard lock(observation.mutex);
    if (!observation.completion ||
        observation.completion->status() != vix::CompletionStatus::Failure)
      return;
    const auto &identity = observation.completion->error().identity();
    const auto domain = identity.domain();
    std::fprintf(stderr, "TLS handshake %s failed: %.*s:%u\n", side,
                 static_cast<int>(domain.size()), domain.data(),
                 static_cast<unsigned>(identity.code()));
  }

  void report_shutdown_observation(const char *side, Observation &observation)
  {
    std::lock_guard lock(observation.mutex);
    if (!observation.completion)
    {
      std::fprintf(stderr, "TLS shutdown %s: pending\n", side);
      return;
    }
    const auto status = observation.completion->status();
    if (status == vix::CompletionStatus::Failure)
    {
      const auto &identity = observation.completion->error().identity();
      const auto domain = identity.domain();
      std::fprintf(stderr, "TLS shutdown %s: status=%d error=%.*s:%u\n", side,
                   static_cast<int>(status), static_cast<int>(domain.size()),
                   domain.data(), static_cast<unsigned>(identity.code()));
      return;
    }
    std::fprintf(stderr, "TLS shutdown %s: status=%d error=none\n", side,
                 static_cast<int>(status));
  }

  void report_shutdown_engine(const char *side, PrivateChannel &channel)
  {
    const auto state = channel.shutdown_diagnostics_for_test();
    const auto &engine = state.engine;
    std::fprintf(stderr,
                 "TLS shutdown %s engine: flags=0x%x sent=%d received=%d "
                 "ssl_result=%d ssl_error=%d "
                 "ciphertext=%zu bio_pending=%zu started=%d complete=%d "
                 "pending=%d want=%d network_generated=%zu network_sent=%zu\n",
                 side, static_cast<unsigned>(engine.ssl_shutdown_flags),
                 (engine.ssl_shutdown_flags & SSL_SENT_SHUTDOWN) != 0,
                 (engine.ssl_shutdown_flags & SSL_RECEIVED_SHUTDOWN) != 0,
                 engine.last_ssl_result, engine.last_ssl_error,
                 engine.last_ciphertext_bytes, engine.bio_pending_bytes,
                 engine.started, engine.complete, engine.pending,
                 static_cast<int>(engine.pending_want),
                 state.network_ciphertext_generated,
                 state.network_ciphertext_sent);
  }

  struct ReadObservation
  {
    std::mutex mutex;
    std::condition_variable changed;
    std::optional<vix::Completion<std::size_t>> completion;

    bool done()
    {
      std::lock_guard lock(mutex);
      return completion.has_value();
    }

    bool wait_for(std::chrono::milliseconds timeout)
    {
      std::unique_lock lock(mutex);
      return changed.wait_for(lock, timeout, [&] { return completion.has_value(); });
    }
  };

  struct Observer
  {
    struct promise_type
    {
      Observer get_return_object() const noexcept { return {}; }
      std::suspend_never initial_suspend() const noexcept { return {}; }
      std::suspend_never final_suspend() const noexcept { return {}; }
      void return_void() const noexcept {}
      void unhandled_exception() const noexcept { std::terminate(); }
    };
  };

  Observer observe(vix::network::Operation<void> operation,
                   std::shared_ptr<Observation> result)
  {
    auto completion = co_await std::move(operation);
    {
      std::lock_guard lock(result->mutex);
      result->completion.emplace(std::move(completion));
    }
    result->changed.notify_all();
  }

  Observer observe_read(vix::network::Operation<std::size_t> operation,
                        std::shared_ptr<ReadObservation> result)
  {
    auto completion = co_await std::move(operation);
    {
      std::lock_guard lock(result->mutex);
      result->completion.emplace(std::move(completion));
    }
    result->changed.notify_all();
  }

  void require_progress(Provider &provider, bool cooperative,
                        const std::shared_ptr<Observation> &first,
                        const std::shared_ptr<Observation> &second,
                        const std::shared_ptr<ReadObservation> &third,
                        const std::shared_ptr<ReadObservation> &fourth)
  {
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 30000; ++turn)
      {
        bool all_done = first->done() && second->done();
        {
          std::lock_guard lock(third->mutex);
          all_done = all_done && third->completion.has_value();
        }
        {
          std::lock_guard lock(fourth->mutex);
          all_done = all_done && fourth->completion.has_value();
        }
        if (all_done)
          return;
        assert(provider.poll());
      }
      assert(false && "cooperative TLS duplex progress exceeded bounded poll count");
      return;
    }
    assert(first->wait_for(std::chrono::seconds(5)));
    assert(second->wait_for(std::chrono::seconds(5)));
    assert(third->wait_for(std::chrono::seconds(5)));
    assert(fourth->wait_for(std::chrono::seconds(5)));
  }

  void memory_bio_handshake_and_retry_contract()
  {
    Contexts contexts;
    auto client_result = OpenSslDriver::create(contexts.client, false, 512);
    auto server_result = OpenSslDriver::create(contexts.server, true, 512);
    assert(client_result && server_result);
    auto client = client_result.take();
    auto server = server_result.take();

    bool client_done = false;
    bool server_done = false;
    std::array<EngineProgress, 2> concurrent_steps{};
    std::barrier start{3};
    std::thread first([&]
                      { start.arrive_and_wait(); concurrent_steps[0] = client->handshake_step(); });
    std::thread second([&]
                       { start.arrive_and_wait(); concurrent_steps[1] = client->handshake_step(); });
    start.arrive_and_wait();
    first.join();
    second.join();
    assert(concurrent_steps[0].want != EngineWant::failure);
    assert(concurrent_steps[1].want != EngineWant::failure);
    bool saw_read_want = concurrent_steps[0].want == EngineWant::read ||
                         concurrent_steps[1].want == EngineWant::read;
    BoundedPipe client_to_server;
    BoundedPipe server_to_client;
    for (unsigned turn = 0; turn < 10000 && !(client_done && server_done); ++turn)
    {
      const auto client_progress = client->handshake_step();
      const auto server_progress = server->handshake_step();
      saw_read_want = saw_read_want || client_progress.want == EngineWant::read ||
                      server_progress.want == EngineWant::read;
      assert(client_progress.want != EngineWant::failure);
      assert(server_progress.want != EngineWant::failure);
      client_done = client_done || client_progress.want == EngineWant::complete;
      server_done = server_done || server_progress.want == EngineWant::complete;
      client_to_server.progress(*client, *server);
      server_to_client.progress(*server, *client);
    }
    assert(client_done && server_done && saw_read_want);

    const auto pending_client_read = client->read_step();
    assert(pending_client_read.want == EngineWant::read);

    std::array<std::byte, 2048> message{};
    for (std::size_t index = 0; index < message.size(); ++index)
      message[index] = static_cast<std::byte>(index % 251U);
    auto write = client->write_step(message);
    bool saw_write_want = write.want == EngineWant::write;
    if (write.want == EngineWant::write)
    {
      auto changed = message;
      changed[0] ^= std::byte{1};
      const auto rejected_retry = client->write_step(changed);
      assert(rejected_retry.want == EngineWant::failure);
      assert(rejected_retry.error == operation_conflict);
    }
    for (unsigned turn = 0; turn < 1000 && write.want != EngineWant::complete; ++turn)
    {
      client_to_server.progress(*client, *server);
      write = client->retry_write();
      saw_write_want = saw_write_want || write.want == EngineWant::write;
      assert(write.want != EngineWant::failure);
    }
    assert(write.want == EngineWant::complete && saw_write_want);
    EngineProgress read{EngineWant::read, 0, backend_failure};
    for (unsigned turn = 0; turn < 1000 && read.want != EngineWant::complete; ++turn)
    {
      client_to_server.progress(*client, *server);
      read = turn == 0 ? server->read_step() : server->retry_read();
      assert(read.want != EngineWant::failure);
    }
    assert(read.want == EngineWant::complete && read.bytes == message.size());
    std::array<std::byte, 2048> plaintext{};
    auto extracted = server->take_plaintext(plaintext);
    assert(extracted && extracted.value() == message.size());
    assert(plaintext == message);

    const std::array<std::byte, 5> reply{
        std::byte{'r'}, std::byte{'e'}, std::byte{'p'}, std::byte{'l'},
        std::byte{'y'}};
    auto reply_write = server->write_step(reply);
    assert(reply_write.want != EngineWant::failure);
    std::array<std::byte, 8> reply_plaintext{};
    EngineProgress reply_read{EngineWant::read, 0, backend_failure};
    for (unsigned turn = 0; turn < 1000 && reply_read.want != EngineWant::complete; ++turn)
    {
      server_to_client.progress(*server, *client);
      if (reply_write.want != EngineWant::complete)
        reply_write = server->retry_write();
      reply_read = client->retry_read();
      assert(reply_write.want != EngineWant::failure);
      assert(reply_read.want != EngineWant::failure);
    }
    assert(reply_write.want == EngineWant::complete);
    assert(reply_read.want == EngineWant::complete && reply_read.bytes == reply.size());
    auto reply_extracted = client->take_plaintext(reply_plaintext);
    assert(reply_extracted && reply_extracted.value() == reply.size());
    assert(std::equal(reply.begin(), reply.end(), reply_plaintext.begin()));

    std::array<std::byte, OpenSslDriver::buffer_capacity> client_wire{};
    std::array<std::byte, OpenSslDriver::buffer_capacity> server_wire{};
    auto deliver = [](EngineProgress progress, std::span<const std::byte> wire,
                      OpenSslDriver &peer)
    {
      if (progress.ciphertext_bytes == 0)
        return;
      const auto bytes = wire.first(progress.ciphertext_bytes);
      auto accepted = peer.feed_ciphertext(bytes);
      assert(accepted && accepted.value() == bytes.size());
    };
    auto client_shutdown = client->shutdown_step(client_wire);
    deliver(client_shutdown, client_wire, *server);
    auto server_shutdown = server->shutdown_step(server_wire);
    deliver(server_shutdown, server_wire, *client);
    assert(client_shutdown.want != EngineWant::failure);
    assert(server_shutdown.want != EngineWant::failure);
    for (unsigned turn = 0; turn < 1000 &&
         (client_shutdown.want != EngineWant::complete ||
          server_shutdown.want != EngineWant::complete); ++turn)
    {
      if (client_shutdown.want != EngineWant::complete)
      {
        client_shutdown = client->retry_shutdown(client_wire);
        deliver(client_shutdown, client_wire, *server);
      }
      if (server_shutdown.want != EngineWant::complete)
      {
        server_shutdown = server->retry_shutdown(server_wire);
        deliver(server_shutdown, server_wire, *client);
      }
      assert(client_shutdown.want != EngineWant::failure);
      assert(server_shutdown.want != EngineWant::failure);
    }
    assert(client_shutdown.want == EngineWant::complete);
    assert(server_shutdown.want == EngineWant::complete);
  }

  void network_children_retire_before_parent(bool cooperative)
  {
    Contexts contexts;
    auto provider_result = cooperative ? Provider::create_cooperative() : Provider::create();
    assert(provider_result);
    auto provider = provider_result.take();
    int sockets[2]{-1, -1};
    assert(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        0, sockets) == 0);
    auto client_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[0]);
    auto server_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[1]);
    sockets[0] = -1;
    sockets[1] = -1;
    auto client_claim = NetworkAccess::claim_tcp_for_tls(std::move(client_connection));
    auto server_claim = NetworkAccess::claim_tcp_for_tls(std::move(server_connection));
    assert(client_claim && server_claim);
    auto client_result = PrivateChannel::create(client_claim.take(), contexts.client, false);
    auto server_result = PrivateChannel::create(server_claim.take(), contexts.server, true);
    assert(client_result && server_result);
    auto client = client_result.take();
    auto server = server_result.take();
    auto client_observation = std::make_shared<Observation>();
    auto server_observation = std::make_shared<Observation>();
    [[maybe_unused]] auto client_observer = observe(client->handshake(), client_observation);
    [[maybe_unused]] auto server_observer = observe(server->handshake(), server_observation);

    if (cooperative)
    {
      for (unsigned turn = 0; turn < 20000 &&
           (!client_observation->done() || !server_observation->done()); ++turn)
        assert(provider.poll());
    }
    else
    {
      assert(client_observation->wait_for(std::chrono::seconds(5)));
      assert(server_observation->wait_for(std::chrono::seconds(5)));
    }
    assert(client_observation->done() && server_observation->done());
    report_handshake_failure("client", *client_observation);
    report_handshake_failure("server", *server_observation);
    assert(client_observation->status() == vix::CompletionStatus::Success);
    assert(server_observation->status() == vix::CompletionStatus::Success);
    client.reset();
    server.reset();
    if (!cooperative)
    {
      const auto core = NetworkAccess::core_for_test(provider);
      const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (core->active_resources_for_test() != 0 &&
             std::chrono::steady_clock::now() < until)
        std::this_thread::yield();
      assert(core->active_resources_for_test() == 0);
    }
    assert(provider.shutdown());

    auto cancel_provider_result = Provider::create_cooperative();
    assert(cancel_provider_result);
    auto cancel_provider = cancel_provider_result.take();
    assert(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        0, sockets) == 0);
    auto pending_connection = NetworkAccess::adopt_tcp_for_test(cancel_provider, sockets[0]);
    auto pending_claim = NetworkAccess::claim_tcp_for_tls(std::move(pending_connection));
    assert(pending_claim);
    auto pending_channel_result = PrivateChannel::create(
        pending_claim.take(), contexts.client, false);
    assert(pending_channel_result);
    auto pending_channel = pending_channel_result.take();
    vix::CancellationSource cancellation;
    auto pending_observation = std::make_shared<Observation>();
    [[maybe_unused]] auto pending_observer = observe(
        pending_channel->handshake(cancellation.token()), pending_observation);
    // Keep the channel alive so its close path cannot mask explicit token cancellation.
    for (unsigned turn = 0; turn < 8; ++turn)
      assert(cancel_provider.poll());
    assert(!pending_observation->done());
    auto retained_lease = cancel_provider.shutdown();
    assert(!retained_lease && retained_lease.error().identity() == provider_in_use);
    assert(cancellation.request_cancel());
    for (unsigned turn = 0; turn < 100 && !pending_observation->done(); ++turn)
      assert(cancel_provider.poll());
    assert(pending_observation->done());
    assert(pending_observation->status() == vix::CompletionStatus::Cancelled);
    pending_channel.reset();
    (void)::close(sockets[1]);
    sockets[1] = -1;
    assert(cancel_provider.shutdown());

    auto deadline_provider_result = Provider::create_cooperative();
    assert(deadline_provider_result);
    auto deadline_provider = deadline_provider_result.take();
    assert(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        0, sockets) == 0);
    auto deadline_connection = NetworkAccess::adopt_tcp_for_test(deadline_provider,
                                                                  sockets[0]);
    auto deadline_claim = NetworkAccess::claim_tcp_for_tls(std::move(deadline_connection));
    assert(deadline_claim);
    auto deadline_channel_result = PrivateChannel::create(
        deadline_claim.take(), contexts.client, false);
    assert(deadline_channel_result);
    auto deadline_channel = deadline_channel_result.take();
    const auto deadline = vix::Deadline::after(vix::Duration::milliseconds(20));
    auto deadline_observation = std::make_shared<Observation>();
    [[maybe_unused]] auto deadline_observer = observe(
        deadline_channel->handshake({}, deadline), deadline_observation);
    // Channel destruction cancels active work; retain it to isolate deadline arbitration.
    for (unsigned turn = 0; turn < 8; ++turn)
      assert(deadline_provider.poll());
    assert(!deadline_observation->done());
    const auto deadline_core = NetworkAccess::core_for_test(deadline_provider);
    assert(deadline_core->active_operations_for_test() >= 2);
    auto deadline_retained = deadline_provider.shutdown();
    assert(!deadline_retained && deadline_retained.error().identity() == provider_in_use);
    const auto wait_bound = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (!deadline.reached() && std::chrono::steady_clock::now() < wait_bound)
      std::this_thread::yield();
    assert(deadline.reached());
    for (unsigned turn = 0; turn < 100 && !deadline_observation->done(); ++turn)
      assert(deadline_provider.poll());
    assert(deadline_observation->done());
    assert(deadline_observation->status() == vix::CompletionStatus::Failure);
    {
      std::lock_guard lock(deadline_observation->mutex);
      assert(deadline_observation->completion->error().identity() == timed_out);
    }
    deadline_channel.reset();
    (void)::close(sockets[1]);
    sockets[1] = -1;
    assert(deadline_provider.shutdown());
  }

  void logical_read_write_full_duplex(bool cooperative)
  {
    Contexts contexts;
    auto provider_result = cooperative ? Provider::create_cooperative() : Provider::create();
    assert(provider_result);
    auto provider = provider_result.take();
    int sockets[2]{-1, -1};
    assert(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        0, sockets) == 0);
    auto client_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[0]);
    auto server_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[1]);
    sockets[0] = sockets[1] = -1;
    auto client_claim = NetworkAccess::claim_tcp_for_tls(std::move(client_connection));
    auto server_claim = NetworkAccess::claim_tcp_for_tls(std::move(server_connection));
    assert(client_claim && server_claim);
    auto client_result = PrivateChannel::create(client_claim.take(), contexts.client, false);
    auto server_result = PrivateChannel::create(server_claim.take(), contexts.server, true);
    assert(client_result && server_result);
    auto client = client_result.take();
    auto server = server_result.take();
    vix::CancellationSource pre_handshake_cancellation;
    assert(pre_handshake_cancellation.request_cancel());
    auto cancelled_handshake = std::make_shared<Observation>();
    [[maybe_unused]] auto cancelled_handshake_task = observe(
        client->handshake(pre_handshake_cancellation.token()),
        cancelled_handshake);
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 100 && !cancelled_handshake->done(); ++turn)
        assert(provider.poll());
    }
    else
      assert(cancelled_handshake->wait_for(std::chrono::seconds(5)));
    assert(cancelled_handshake->status() == vix::CompletionStatus::Cancelled);

    auto client_handshake = std::make_shared<Observation>();
    auto server_handshake = std::make_shared<Observation>();
    [[maybe_unused]] auto client_handshake_task = observe(client->handshake(), client_handshake);
    [[maybe_unused]] auto server_handshake_task = observe(server->handshake(), server_handshake);
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 30000 &&
           (!client_handshake->done() || !server_handshake->done()); ++turn)
        assert(provider.poll());
    }
    else
    {
      assert(client_handshake->wait_for(std::chrono::seconds(5)));
      assert(server_handshake->wait_for(std::chrono::seconds(5)));
    }
    assert(client_handshake->status() == vix::CompletionStatus::Success);
    assert(server_handshake->status() == vix::CompletionStatus::Success);

    vix::CancellationSource read_cancellation;
    auto cancelled_read = std::make_shared<ReadObservation>();
    const auto core = NetworkAccess::core_for_test(provider);
    const auto operation_baseline = core->active_operations_for_test();
    std::array<std::byte, 4> cancelled_buffer{};
    [[maybe_unused]] auto cancelled_read_task = observe_read(
        server->read(cancelled_buffer, read_cancellation.token()), cancelled_read);
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 8; ++turn)
        assert(provider.poll());
    }
    else
    {
      const auto armed_until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (core->active_operations_for_test() < operation_baseline + 2 &&
             std::chrono::steady_clock::now() < armed_until)
        std::this_thread::yield();
    }
    assert(core->active_operations_for_test() >= operation_baseline + 2);
    {
      std::lock_guard lock(cancelled_read->mutex);
      assert(!cancelled_read->completion);
    }
    assert(read_cancellation.request_cancel());
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 100 && !cancelled_read->done(); ++turn)
        assert(provider.poll());
    }
    else
      assert(cancelled_read->wait_for(std::chrono::seconds(5)));
    {
      std::lock_guard lock(cancelled_read->mutex);
      assert(cancelled_read->completion);
      assert(cancelled_read->completion->status() == vix::CompletionStatus::Cancelled);
    }

    const auto read_deadline = vix::Deadline::after(vix::Duration::milliseconds(20));
    auto timed_read = std::make_shared<ReadObservation>();
    const auto before_timed_read = core->active_operations_for_test();
    [[maybe_unused]] auto timed_read_task = observe_read(
        server->read(cancelled_buffer, {}, read_deadline), timed_read);
    const auto timed_read_armed_until = std::chrono::steady_clock::now() +
                                        std::chrono::seconds(2);
    while (core->active_operations_for_test() < before_timed_read + 2 &&
           std::chrono::steady_clock::now() < timed_read_armed_until)
      std::this_thread::yield();
    assert(core->active_operations_for_test() >= before_timed_read + 2);
    const auto deadline_until = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (!read_deadline.reached() && std::chrono::steady_clock::now() < deadline_until)
      std::this_thread::yield();
    assert(read_deadline.reached());
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 1000 && !timed_read->done(); ++turn)
        assert(provider.poll());
    }
    else
    {
      assert(timed_read->wait_for(std::chrono::seconds(5)));
    }
    {
      std::lock_guard lock(timed_read->mutex);
      assert(timed_read->completion);
      assert(timed_read->completion->status() == vix::CompletionStatus::Failure);
      assert(timed_read->completion->error().identity() == timed_out);
    }

    std::array<std::byte, 7> from_client{
        std::byte{'c'}, std::byte{'l'}, std::byte{'i'}, std::byte{'e'},
        std::byte{'n'}, std::byte{'t'}, std::byte{'!'}};
    std::array<std::byte, 8> from_server{
        std::byte{'s'}, std::byte{'e'}, std::byte{'r'}, std::byte{'v'},
        std::byte{'e'}, std::byte{'r'}, std::byte{'!'}, std::byte{'!'}};
    std::array<std::byte, 16> client_input{};
    std::array<std::byte, 16> server_input{};
    auto client_write = std::make_shared<Observation>();
    auto server_write = std::make_shared<Observation>();
    auto client_read = std::make_shared<ReadObservation>();
    auto server_read = std::make_shared<ReadObservation>();
    [[maybe_unused]] auto client_read_task = observe_read(client->read(client_input), client_read);
    [[maybe_unused]] auto server_read_task = observe_read(server->read(server_input), server_read);
    [[maybe_unused]] auto client_write_task = observe(client->write(from_client), client_write);
    [[maybe_unused]] auto server_write_task = observe(server->write(from_server), server_write);
    if (cooperative)
    {
      assert(!client_write->done() && !server_write->done());
      assert(!client_read->done() && !server_read->done());
    }
    require_progress(provider, cooperative, client_write, server_write,
                     client_read, server_read);
    assert(client_write->status() == vix::CompletionStatus::Success);
    assert(server_write->status() == vix::CompletionStatus::Success);
    {
      std::lock_guard lock(client_read->mutex);
      assert(client_read->completion->status() == vix::CompletionStatus::Success);
      assert(client_read->completion->take() == from_server.size());
    }
    {
      std::lock_guard lock(server_read->mutex);
      assert(server_read->completion->status() == vix::CompletionStatus::Success);
      assert(server_read->completion->take() == from_client.size());
    }
    assert(std::equal(from_server.begin(), from_server.end(), client_input.begin()));
    assert(std::equal(from_client.begin(), from_client.end(), server_input.begin()));
    if (cooperative)
    {
      std::array<std::byte, OpenSslDriver::buffer_capacity> cancelled_payload{};
      vix::CancellationSource write_cancellation;
      auto cancelled_write = std::make_shared<Observation>();
      [[maybe_unused]] auto cancelled_write_task = observe(
          client->write(cancelled_payload, write_cancellation.token()), cancelled_write);
      assert(!cancelled_write->done());
      const auto pending_write_until = std::chrono::steady_clock::now() +
                                       std::chrono::seconds(2);
      while (core->active_operations_for_test() < 2 &&
             std::chrono::steady_clock::now() < pending_write_until)
        std::this_thread::yield();
      assert(core->active_operations_for_test() >= 2);
      assert(write_cancellation.request_cancel());
      for (unsigned turn = 0; turn < 100 && !cancelled_write->done(); ++turn)
        assert(provider.poll());
      assert(cancelled_write->done());
      assert(cancelled_write->status() == vix::CompletionStatus::Cancelled);
    }
    client.reset();
    server.reset();
    if (!cooperative)
    {
      const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (core->active_resources_for_test() != 0 &&
             std::chrono::steady_clock::now() < until)
        std::this_thread::yield();
      assert(core->active_resources_for_test() == 0);
    }
    assert(provider.shutdown());
  }

  void private_channel_graceful_shutdown(bool cooperative)
  {
    Contexts contexts;
    auto provider_result = cooperative ? Provider::create_cooperative() : Provider::create();
    assert(provider_result);
    auto provider = provider_result.take();
    int sockets[2]{-1, -1};
    assert(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        0, sockets) == 0);
    auto client_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[0]);
    auto server_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[1]);
    sockets[0] = sockets[1] = -1;
    auto client_claim = NetworkAccess::claim_tcp_for_tls(std::move(client_connection));
    auto server_claim = NetworkAccess::claim_tcp_for_tls(std::move(server_connection));
    assert(client_claim && server_claim);
    auto client_result = PrivateChannel::create(client_claim.take(), contexts.client, false);
    auto server_result = PrivateChannel::create(server_claim.take(), contexts.server, true);
    assert(client_result && server_result);
    auto client = client_result.take();
    auto server = server_result.take();
    auto client_handshake = std::make_shared<Observation>();
    auto server_handshake = std::make_shared<Observation>();
    [[maybe_unused]] auto client_handshake_task = observe(client->handshake(), client_handshake);
    [[maybe_unused]] auto server_handshake_task = observe(server->handshake(), server_handshake);
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 30000 &&
           (!client_handshake->done() || !server_handshake->done()); ++turn)
        assert(provider.poll());
    }
    else
    {
      assert(client_handshake->wait_for(std::chrono::seconds(5)));
      assert(server_handshake->wait_for(std::chrono::seconds(5)));
    }
    assert(client_handshake->status() == vix::CompletionStatus::Success);
    assert(server_handshake->status() == vix::CompletionStatus::Success);

    auto server_shutdown = std::make_shared<Observation>();
    [[maybe_unused]] auto server_shutdown_task = observe(server->shutdown(), server_shutdown);
    std::array<std::byte, 4> peer_eof_buffer{};
    auto peer_close_observation = std::make_shared<ReadObservation>();
    [[maybe_unused]] auto peer_close_task = observe_read(
        client->read(peer_eof_buffer), peer_close_observation);
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 30000 && !peer_close_observation->done(); ++turn)
        assert(provider.poll());
    }
    else
      assert(peer_close_observation->wait_for(std::chrono::seconds(5)));
    {
      std::lock_guard lock(peer_close_observation->mutex);
      assert(peer_close_observation->completion);
      assert(peer_close_observation->completion->status() ==
             vix::CompletionStatus::Failure);
      assert(peer_close_observation->completion->error().identity() ==
             vix::tls::peer_closed);
    }

    auto client_shutdown = std::make_shared<Observation>();
    [[maybe_unused]] auto client_shutdown_task = observe(client->shutdown(), client_shutdown);
    bool client_shutdown_ready = true;
    bool server_shutdown_ready = true;
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 30000 &&
           (!client_shutdown->done() || !server_shutdown->done()); ++turn)
        assert(provider.poll());
      client_shutdown_ready = client_shutdown->done();
      server_shutdown_ready = server_shutdown->done();
    }
    else
    {
      client_shutdown_ready = client_shutdown->wait_for(std::chrono::seconds(5));
      server_shutdown_ready = server_shutdown->wait_for(std::chrono::seconds(5));
    }
    const bool client_shutdown_success = client_shutdown_ready &&
        client_shutdown->status() == vix::CompletionStatus::Success;
    const bool server_shutdown_success = server_shutdown_ready &&
        server_shutdown->status() == vix::CompletionStatus::Success;
    if (!client_shutdown_success || !server_shutdown_success)
    {
      std::fprintf(stderr, "TLS graceful shutdown mode=%s\n",
                   cooperative ? "cooperative" : "background");
      report_shutdown_observation("client", *client_shutdown);
      report_shutdown_observation("server", *server_shutdown);
      report_shutdown_engine("client", *client);
      report_shutdown_engine("server", *server);
      const auto core = NetworkAccess::core_for_test(provider);
      std::fprintf(stderr,
                   "TLS shutdown Network: active_operations=%u active_resources=%u\n",
                   core->active_operations_for_test(),
                   core->active_resources_for_test());
    }
    assert(client_shutdown_ready);
    assert(server_shutdown_ready);
    assert(client_shutdown->status() == vix::CompletionStatus::Success);
    {
      std::lock_guard lock(server_shutdown->mutex);
      assert(server_shutdown->completion);
      assert(server_shutdown->completion->status() ==
             vix::CompletionStatus::Success);
    }
    assert(server_shutdown->status() == vix::CompletionStatus::Success);
    client.reset();
    server.reset();
    if (!cooperative)
    {
      const auto core = NetworkAccess::core_for_test(provider);
      const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (core->active_resources_for_test() != 0 &&
             std::chrono::steady_clock::now() < until)
        std::this_thread::yield();
      assert(core->active_resources_for_test() == 0);
    }
    assert(provider.shutdown());
  }

  void private_channel_detects_truncated_eof(bool cooperative)
  {
    Contexts contexts;
    auto provider_result = cooperative ? Provider::create_cooperative() : Provider::create();
    assert(provider_result);
    auto provider = provider_result.take();
    int sockets[2]{-1, -1};
    assert(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        0, sockets) == 0);
    auto client_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[0]);
    auto server_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[1]);
    sockets[0] = sockets[1] = -1;
    auto client_claim = NetworkAccess::claim_tcp_for_tls(std::move(client_connection));
    auto server_claim = NetworkAccess::claim_tcp_for_tls(std::move(server_connection));
    assert(client_claim && server_claim);
    auto client_result = PrivateChannel::create(client_claim.take(), contexts.client, false);
    auto server_result = PrivateChannel::create(server_claim.take(), contexts.server, true);
    assert(client_result && server_result);
    auto client = client_result.take();
    auto server = server_result.take();
    auto client_handshake = std::make_shared<Observation>();
    auto server_handshake = std::make_shared<Observation>();
    [[maybe_unused]] auto client_handshake_task = observe(client->handshake(), client_handshake);
    [[maybe_unused]] auto server_handshake_task = observe(server->handshake(), server_handshake);
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 30000 &&
           (!client_handshake->done() || !server_handshake->done()); ++turn)
        assert(provider.poll());
    }
    else
    {
      assert(client_handshake->wait_for(std::chrono::seconds(5)));
      assert(server_handshake->wait_for(std::chrono::seconds(5)));
    }
    assert(client_handshake->status() == vix::CompletionStatus::Success);
    assert(server_handshake->status() == vix::CompletionStatus::Success);

    std::array<std::byte, 8> output{};
    auto read_observation = std::make_shared<ReadObservation>();
    [[maybe_unused]] auto read_task = observe_read(client->read(output), read_observation);
    if (cooperative)
      assert(provider.poll());
    else
    {
      const auto core = NetworkAccess::core_for_test(provider);
      const auto armed_until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (core->active_operations_for_test() < 2 &&
             std::chrono::steady_clock::now() < armed_until)
        std::this_thread::yield();
      assert(core->active_operations_for_test() >= 2);
    }
    server->close(); // Raw FIN without a TLS close_notify.
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 1000 && !read_observation->done(); ++turn)
        assert(provider.poll());
    }
    else
      assert(read_observation->wait_for(std::chrono::seconds(5)));
    {
      std::lock_guard lock(read_observation->mutex);
      assert(read_observation->completion);
      assert(read_observation->completion->status() == vix::CompletionStatus::Failure);
      assert(read_observation->completion->error().identity() == vix::tls::peer_truncated);
    }
    client.reset();
    server.reset();
    if (!cooperative)
    {
      const auto core = NetworkAccess::core_for_test(provider);
      const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (core->active_resources_for_test() != 0 &&
             std::chrono::steady_clock::now() < until)
        std::this_thread::yield();
      assert(core->active_resources_for_test() == 0);
    }
    assert(provider.shutdown());
  }

  void public_authenticated_channel(bool cooperative, bool identity_matches,
                                    bool trust_fixture, bool identity_is_ip = false)
  {
    Contexts credentials;
    PemCredentials pem{credentials};
    vix::tls::TlsClientOptions client_options;
    client_options.expected_peer = {
        identity_is_ip ? vix::tls::PeerIdentityKind::ip_address
                       : vix::tls::PeerIdentityKind::dns_name,
        identity_is_ip
            ? (identity_matches ? "127.0.0.1" : "127.0.0.2")
            : (identity_matches ? "vix-test.invalid" : "wrong-test.invalid")};
    client_options.use_system_trust = trust_fixture;
    if (!trust_fixture)
      client_options.ca_file = pem.certificate_path;
    // An explicit SNI value differs from the reference identity to check that
    // SNI is not accidentally used as the certificate verification name.
    client_options.server_name = "sni-test.invalid";
    auto client_config = vix::tls::TlsClientConfig::create(std::move(client_options));
    assert(client_config);
    vix::tls::TlsServerOptions server_options;
    server_options.certificate_chain_file = pem.certificate_path;
    server_options.private_key_file = pem.key_path;
    auto server_config = vix::tls::TlsServerConfig::create(std::move(server_options));
    assert(server_config);

    auto provider_result = cooperative ? Provider::create_cooperative() : Provider::create();
    assert(provider_result);
    auto provider = provider_result.take();
    int sockets[2]{-1, -1};
    assert(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        0, sockets) == 0);
    auto client_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[0]);
    auto server_connection = NetworkAccess::adopt_tcp_for_test(provider, sockets[1]);
    sockets[0] = sockets[1] = -1;
    auto client_result = vix::tls::TlsChannel::create_client(
        std::move(client_connection), client_config.value());
    auto server_result = vix::tls::TlsChannel::create_server(
        std::move(server_connection), server_config.value());
    assert(client_result && server_result);
    auto client = client_result.take();
    auto server = server_result.take();

    std::array<std::byte, 4> preauth_buffer{};
    auto preauth_receive = std::make_shared<ReadObservation>();
    auto preauth_send = std::make_shared<Observation>();
    [[maybe_unused]] auto preauth_receive_task = observe_read(
        client.receive(preauth_buffer), preauth_receive);
    [[maybe_unused]] auto preauth_send_task = observe(
        client.send(preauth_buffer), preauth_send);
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 100 &&
           (!preauth_receive->done() || !preauth_send->done()); ++turn)
        assert(provider.poll());
    }
    else
    {
      assert(preauth_receive->wait_for(std::chrono::seconds(5)));
      assert(preauth_send->wait_for(std::chrono::seconds(5)));
    }
    {
      std::lock_guard lock(preauth_receive->mutex);
      assert(preauth_receive->completion);
      assert(preauth_receive->completion->status() == vix::CompletionStatus::Failure);
      assert(preauth_receive->completion->error().identity() == vix::tls::channel_closed);
    }
    assert(preauth_send->status() == vix::CompletionStatus::Failure);
    {
      std::lock_guard lock(preauth_send->mutex);
      assert(preauth_send->completion->error().identity() == vix::tls::channel_closed);
    }

    auto client_handshake = std::make_shared<Observation>();
    auto server_handshake = std::make_shared<Observation>();
    [[maybe_unused]] auto client_handshake_task = observe(client.handshake(), client_handshake);
    [[maybe_unused]] auto server_handshake_task = observe(server.handshake(), server_handshake);
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 30000 &&
           (!client_handshake->done() || !server_handshake->done()); ++turn)
        assert(provider.poll());
    }
    else
    {
      assert(client_handshake->wait_for(std::chrono::seconds(5)));
      assert(server_handshake->wait_for(std::chrono::seconds(5)));
    }
    if (!identity_matches || trust_fixture)
    {
      {
        std::lock_guard lock(client_handshake->mutex);
        assert(client_handshake->completion);
        assert(client_handshake->completion->status() == vix::CompletionStatus::Failure);
        if (!identity_matches)
          assert(client_handshake->completion->error().identity() ==
                 vix::tls::peer_identity_mismatch);
        else
          assert(client_handshake->completion->error().identity() ==
                 vix::tls::authentication_failed);
      }
      client.close();
      server.close();
    }
    else
    {
      assert(client_handshake->status() == vix::CompletionStatus::Success);
      assert(server_handshake->status() == vix::CompletionStatus::Success);
      std::array<std::byte, OpenSslDriver::buffer_capacity + 1> oversized_plaintext{};
      auto oversized_send = std::make_shared<Observation>();
      [[maybe_unused]] auto oversized_task = observe(
          client.send(oversized_plaintext), oversized_send);
      if (cooperative)
      {
        for (unsigned turn = 0; turn < 100 && !oversized_send->done(); ++turn)
          assert(provider.poll());
      }
      else
        assert(oversized_send->wait_for(std::chrono::seconds(5)));
      assert(oversized_send->status() == vix::CompletionStatus::Failure);
      {
        std::lock_guard lock(oversized_send->mutex);
        assert(oversized_send->completion->error().identity() ==
               vix::tls::resource_limit);
      }
      std::array<std::byte, 9> plaintext{
          std::byte{'s'}, std::byte{'e'}, std::byte{'c'}, std::byte{'u'},
          std::byte{'r'}, std::byte{'e'}, std::byte{'!'}, std::byte{'!'},
          std::byte{'!'}};
      std::array<std::byte, 16> received{};
      auto send_observation = std::make_shared<Observation>();
      auto receive_observation = std::make_shared<ReadObservation>();
      [[maybe_unused]] auto send_task = observe(client.send(plaintext), send_observation);
      [[maybe_unused]] auto receive_task = observe_read(
          server.receive(received), receive_observation);
      if (cooperative)
      {
        for (unsigned turn = 0; turn < 30000 &&
             (!send_observation->done() || !receive_observation->done()); ++turn)
          assert(provider.poll());
      }
      else
      {
        assert(send_observation->wait_for(std::chrono::seconds(5)));
        assert(receive_observation->wait_for(std::chrono::seconds(5)));
      }
      assert(send_observation->status() == vix::CompletionStatus::Success);
      {
        std::lock_guard lock(receive_observation->mutex);
        assert(receive_observation->completion);
        assert(receive_observation->completion->status() ==
               vix::CompletionStatus::Success);
        assert(receive_observation->completion->take() == plaintext.size());
      }
      assert(std::equal(plaintext.begin(), plaintext.end(), received.begin()));
      auto client_shutdown = std::make_shared<Observation>();
      auto server_shutdown = std::make_shared<Observation>();
      [[maybe_unused]] auto client_shutdown_task = observe(
          client.shutdown(), client_shutdown);
      [[maybe_unused]] auto server_shutdown_task = observe(
          server.shutdown(), server_shutdown);
      if (cooperative)
      {
        for (unsigned turn = 0; turn < 30000 &&
             (!client_shutdown->done() || !server_shutdown->done()); ++turn)
          assert(provider.poll());
      }
      else
      {
        assert(client_shutdown->wait_for(std::chrono::seconds(5)));
        assert(server_shutdown->wait_for(std::chrono::seconds(5)));
      }
      assert(client_shutdown->status() == vix::CompletionStatus::Success);
      assert(server_shutdown->status() == vix::CompletionStatus::Success);
    }
    if (cooperative)
    {
      for (unsigned turn = 0; turn < 1000; ++turn)
        assert(provider.poll());
    }
    else
    {
      const auto core = NetworkAccess::core_for_test(provider);
      const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (core->active_resources_for_test() != 0 &&
             std::chrono::steady_clock::now() < until)
        std::this_thread::yield();
      assert(core->active_resources_for_test() == 0);
    }
    assert(provider.shutdown());
  }
}

int main()
{
  vix::tls::TlsClientOptions invalid_client_options{};
  assert(!vix::tls::TlsClientConfig::create(std::move(invalid_client_options)));
  vix::tls::TlsServerOptions invalid_server_options{};
  assert(!vix::tls::TlsServerConfig::create(std::move(invalid_server_options)));
  memory_bio_handshake_and_retry_contract();
  network_children_retire_before_parent(true);
  network_children_retire_before_parent(false);
  logical_read_write_full_duplex(true);
  logical_read_write_full_duplex(false);
  private_channel_graceful_shutdown(true);
  private_channel_graceful_shutdown(false);
  private_channel_detects_truncated_eof(true);
  private_channel_detects_truncated_eof(false);
  public_authenticated_channel(true, true, false);
  public_authenticated_channel(false, true, false);
  public_authenticated_channel(true, false, false);
  public_authenticated_channel(false, false, false);
  public_authenticated_channel(true, true, true);
  public_authenticated_channel(false, true, true);
  public_authenticated_channel(true, true, false, true);
  public_authenticated_channel(false, true, false, true);
  public_authenticated_channel(true, false, false, true);
}
