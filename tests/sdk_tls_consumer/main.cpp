#include <vix/Completion.hpp>
#include <vix/tls/Errors.hpp>
#include <vix/tls/TlsChannel.hpp>
#include <vix/tls/TlsConfig.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpListener.hpp>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>

namespace
{
  struct TestCredentials
  {
    std::string certificate_path;
    std::string key_path;

    TestCredentials()
    {
      EVP_PKEY_CTX *key_context = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
      assert(key_context != nullptr);
      assert(EVP_PKEY_keygen_init(key_context) == 1);
      assert(EVP_PKEY_CTX_set_rsa_keygen_bits(key_context, 2048) == 1);
      EVP_PKEY *key{};
      assert(EVP_PKEY_keygen(key_context, &key) == 1);
      EVP_PKEY_CTX_free(key_context);

      X509 *certificate = X509_new();
      assert(certificate != nullptr);
      assert(X509_set_version(certificate, 2) == 1);
      assert(ASN1_INTEGER_set(X509_get_serialNumber(certificate), 7) == 1);
      assert(X509_gmtime_adj(X509_getm_notBefore(certificate), -60) != nullptr);
      assert(X509_gmtime_adj(X509_getm_notAfter(certificate), 3600) != nullptr);
      assert(X509_set_pubkey(certificate, key) == 1);
      X509_NAME *name = X509_get_subject_name(certificate);
      constexpr unsigned char common_name[] = "localhost";
      assert(X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, common_name,
                                        -1, -1, 0) == 1);
      assert(X509_set_issuer_name(certificate, name) == 1);
      X509V3_CTX extension_context{};
      X509V3_set_ctx(&extension_context, certificate, certificate, nullptr,
                     nullptr, 0);
      char constraints[] = "critical,CA:TRUE";
      X509_EXTENSION *basic_constraints = X509V3_EXT_conf_nid(
          nullptr, &extension_context, NID_basic_constraints, constraints);
      assert(basic_constraints != nullptr);
      assert(X509_add_ext(certificate, basic_constraints, -1) == 1);
      X509_EXTENSION_free(basic_constraints);
      char subject_alt_name[] = "DNS:localhost";
      X509_EXTENSION *san = X509V3_EXT_conf_nid(
          nullptr, &extension_context, NID_subject_alt_name, subject_alt_name);
      assert(san != nullptr);
      assert(X509_add_ext(certificate, san, -1) == 1);
      X509_EXTENSION_free(san);
      assert(X509_sign(certificate, key, EVP_sha256()) > 0);

      char certificate_template[] = "/tmp/vix-tls-sdk-cert-XXXXXX";
      const int certificate_fd = ::mkstemp(certificate_template);
      assert(certificate_fd >= 0);
      certificate_path = certificate_template;
      FILE *certificate_file = ::fdopen(certificate_fd, "w");
      assert(certificate_file != nullptr);
      assert(PEM_write_X509(certificate_file, certificate) == 1);
      assert(std::fclose(certificate_file) == 0);

      char key_template[] = "/tmp/vix-tls-sdk-key-XXXXXX";
      const int key_fd = ::mkstemp(key_template);
      assert(key_fd >= 0);
      key_path = key_template;
      FILE *key_file = ::fdopen(key_fd, "w");
      assert(key_file != nullptr);
      assert(PEM_write_PrivateKey(key_file, key, nullptr, nullptr, 0, nullptr,
                                  nullptr) == 1);
      assert(std::fclose(key_file) == 0);

      X509_free(certificate);
      EVP_PKEY_free(key);
    }

    ~TestCredentials()
    {
      if (!certificate_path.empty())
        (void)::unlink(certificate_path.c_str());
      if (!key_path.empty())
        (void)::unlink(key_path.c_str());
    }
  };

  struct Observation
  {
    std::mutex mutex;
    std::condition_variable changed;
    std::optional<vix::Completion<void>> completion;

    void publish(vix::Completion<void> value)
    {
      {
        std::lock_guard lock(mutex);
        assert(!completion);
        completion.emplace(std::move(value));
      }
      changed.notify_all();
    }

    bool wait()
    {
      std::unique_lock lock(mutex);
      return changed.wait_for(lock, std::chrono::seconds(10),
                              [&] { return completion.has_value(); });
    }

    vix::CompletionStatus status()
    {
      std::lock_guard lock(mutex);
      assert(completion);
      return completion->status();
    }
  };

  struct Detached
  {
    struct promise_type
    {
      Detached get_return_object() const noexcept { return {}; }
      std::suspend_never initial_suspend() const noexcept { return {}; }
      std::suspend_never final_suspend() const noexcept { return {}; }
      void return_void() const noexcept {}
      void unhandled_exception() const noexcept { std::terminate(); }
    };
  };

  Detached serve(std::shared_ptr<vix::network::TcpListener> listener,
                 vix::tls::TlsServerConfig config,
                 std::shared_ptr<Observation> observation)
  {
    auto accepted = co_await listener->accept();
    if (accepted.status() != vix::CompletionStatus::Success)
    {
      observation->publish(accepted.status() == vix::CompletionStatus::Failure
                               ? vix::Completion<void>{accepted.error()}
                               : vix::Completion<void>::cancelled());
      co_return;
    }
    auto channel_result = vix::tls::TlsChannel::create_server(
        accepted.take(), config);
    if (!channel_result)
    {
      observation->publish(vix::Completion<void>{channel_result.error()});
      co_return;
    }
    auto channel = channel_result.take();
    auto handshake = co_await channel.handshake();
    if (handshake.status() != vix::CompletionStatus::Success)
    {
      observation->publish(std::move(handshake));
      co_return;
    }
    auto shutdown = co_await channel.shutdown();
    observation->publish(std::move(shutdown));
  }

  Detached connect_and_handshake(vix::network::Provider *provider,
                                 vix::network::Endpoint endpoint,
                                 vix::tls::TlsClientConfig config,
                                 std::shared_ptr<Observation> observation)
  {
    auto connected = co_await provider->connect(endpoint);
    if (connected.status() != vix::CompletionStatus::Success)
    {
      observation->publish(connected.status() == vix::CompletionStatus::Failure
                               ? vix::Completion<void>{connected.error()}
                               : vix::Completion<void>::cancelled());
      co_return;
    }
    auto channel_result = vix::tls::TlsChannel::create_client(
        connected.take(), config);
    if (!channel_result)
    {
      observation->publish(vix::Completion<void>{channel_result.error()});
      co_return;
    }
    auto channel = channel_result.take();
    auto handshake = co_await channel.handshake();
    if (handshake.status() != vix::CompletionStatus::Success)
    {
      observation->publish(std::move(handshake));
      co_return;
    }
    auto shutdown = co_await channel.shutdown();
    observation->publish(std::move(shutdown));
  }
}

int main()
{
  TestCredentials credentials;
  vix::tls::TlsClientOptions client_options;
  client_options.expected_peer = {vix::tls::PeerIdentityKind::dns_name,
                                  "localhost"};
  client_options.use_system_trust = false;
  client_options.ca_file = credentials.certificate_path;
  auto client_config = vix::tls::TlsClientConfig::create(std::move(client_options));
  assert(client_config);

  vix::tls::TlsServerOptions server_options;
  server_options.certificate_chain_file = credentials.certificate_path;
  server_options.private_key_file = credentials.key_path;
  auto server_config = vix::tls::TlsServerConfig::create(std::move(server_options));
  assert(server_config);

  auto provider_result = vix::network::Provider::create();
  assert(provider_result);
  auto provider = provider_result.take();
  auto address = vix::network::Address::parse("127.0.0.1");
  assert(address);
  auto endpoint = vix::network::Endpoint::create(address.take(), 0);
  assert(endpoint);
  auto listener_result = provider.listen(endpoint.take());
  assert(listener_result);
  auto listener = std::make_shared<vix::network::TcpListener>(
      listener_result.take());
  auto local_endpoint = listener->local_endpoint();
  assert(local_endpoint);

  auto client_observation = std::make_shared<Observation>();
  auto server_observation = std::make_shared<Observation>();
  [[maybe_unused]] auto server_task = serve(listener, server_config.take(),
                                            server_observation);
  [[maybe_unused]] auto client_task = connect_and_handshake(
      &provider, local_endpoint.take(), client_config.take(), client_observation);
  assert(client_observation->wait());
  assert(server_observation->wait());
  assert(client_observation->status() == vix::CompletionStatus::Success);
  assert(server_observation->status() == vix::CompletionStatus::Success);
  assert(listener->close());
  assert(provider.shutdown());
}
