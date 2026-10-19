#include <vix/http/Https.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpListener.hpp>
#include <vix/Deadline.hpp>
#include <vix/Duration.hpp>
#include <vix/MonotonicClock.hpp>
#include <vix/tls/Errors.hpp>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>

namespace
{
  void require(bool condition)
  {
    if (!condition)
      std::abort();
  }

  std::vector<std::byte> bytes(std::string_view text)
  {
    std::vector<std::byte> result;
    for (const unsigned char value : text)
      result.push_back(static_cast<std::byte>(value));
    return result;
  }

  std::string string_of(const std::vector<std::byte> &value)
  {
    std::string result;
    for (const auto byte : value)
      result.push_back(static_cast<char>(std::to_integer<unsigned char>(byte)));
    return result;
  }

  struct Credentials
  {
    std::string certificate_path;
    std::string key_path;

    Credentials()
    {
      EVP_PKEY_CTX *key_context = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
      require(key_context != nullptr);
      require(EVP_PKEY_keygen_init(key_context) == 1);
      require(EVP_PKEY_CTX_set_rsa_keygen_bits(key_context, 2048) == 1);
      EVP_PKEY *key{};
      require(EVP_PKEY_keygen(key_context, &key) == 1);
      EVP_PKEY_CTX_free(key_context);

      X509 *certificate = X509_new();
      require(certificate != nullptr);
      require(X509_set_version(certificate, 2) == 1);
      require(ASN1_INTEGER_set(X509_get_serialNumber(certificate), 91) == 1);
      require(X509_gmtime_adj(X509_getm_notBefore(certificate), -60) != nullptr);
      require(X509_gmtime_adj(X509_getm_notAfter(certificate), 3600) != nullptr);
      require(X509_set_pubkey(certificate, key) == 1);
      X509_NAME *name = X509_get_subject_name(certificate);
      constexpr unsigned char common_name[] = "localhost";
      require(X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, common_name,
                                         -1, -1, 0) == 1);
      require(X509_set_issuer_name(certificate, name) == 1);
      X509V3_CTX extension_context{};
      X509V3_set_ctx(&extension_context, certificate, certificate, nullptr, nullptr, 0);
      char constraints[] = "critical,CA:TRUE";
      X509_EXTENSION *basic_constraints = X509V3_EXT_conf_nid(
          nullptr, &extension_context, NID_basic_constraints, constraints);
      require(basic_constraints != nullptr);
      require(X509_add_ext(certificate, basic_constraints, -1) == 1);
      X509_EXTENSION_free(basic_constraints);
      char subject_alt_name[] = "DNS:localhost";
      X509_EXTENSION *san = X509V3_EXT_conf_nid(
          nullptr, &extension_context, NID_subject_alt_name, subject_alt_name);
      require(san != nullptr);
      require(X509_add_ext(certificate, san, -1) == 1);
      X509_EXTENSION_free(san);
      require(X509_sign(certificate, key, EVP_sha256()) > 0);

      char certificate_template[] = "/tmp/vix-http-tls-cert-XXXXXX";
      const int certificate_fd = ::mkstemp(certificate_template);
      require(certificate_fd >= 0);
      certificate_path = certificate_template;
      FILE *certificate_file = ::fdopen(certificate_fd, "w");
      require(certificate_file != nullptr);
      require(PEM_write_X509(certificate_file, certificate) == 1);
      require(std::fclose(certificate_file) == 0);

      char key_template[] = "/tmp/vix-http-tls-key-XXXXXX";
      const int key_fd = ::mkstemp(key_template);
      require(key_fd >= 0);
      key_path = key_template;
      FILE *key_file = ::fdopen(key_fd, "w");
      require(key_file != nullptr);
      require(PEM_write_PrivateKey(key_file, key, nullptr, nullptr, 0, nullptr, nullptr) == 1);
      require(std::fclose(key_file) == 0);
      X509_free(certificate);
      EVP_PKEY_free(key);
    }

    ~Credentials()
    {
      if (!certificate_path.empty()) (void)::unlink(certificate_path.c_str());
      if (!key_path.empty()) (void)::unlink(key_path.c_str());
    }
  };

  vix::tls::TlsServerConfig server_config(const Credentials &credentials)
  {
    vix::tls::TlsServerOptions options;
    options.certificate_chain_file = credentials.certificate_path;
    options.private_key_file = credentials.key_path;
    auto config = vix::tls::TlsServerConfig::create(std::move(options));
    require(static_cast<bool>(config));
    return config.take();
  }

  vix::tls::TlsClientConfig client_config(const Credentials &credentials,
                                         std::string expected_identity)
  {
    vix::tls::TlsClientOptions options;
    options.expected_peer = {vix::tls::PeerIdentityKind::dns_name,
                             std::move(expected_identity)};
    options.ca_file = credentials.certificate_path;
    options.use_system_trust = false;
    auto config = vix::tls::TlsClientConfig::create(std::move(options));
    require(static_cast<bool>(config));
    return config.take();
  }

  vix::tls::TlsClientConfig system_trust_client_config()
  {
    vix::tls::TlsClientOptions options;
    options.expected_peer = {vix::tls::PeerIdentityKind::dns_name, "localhost"};
    auto config = vix::tls::TlsClientConfig::create(std::move(options));
    require(static_cast<bool>(config));
    return config.take();
  }

  void authenticated_exchange_and_identity_failure()
  {
    Credentials credentials;
    auto tls_server = server_config(credentials);
    auto tls_client = client_config(credentials, "localhost");
    auto provider_result = vix::network::Provider::create();
    require(static_cast<bool>(provider_result));
    auto provider = provider_result.take();
    auto address = vix::network::Address::parse("127.0.0.1");
    require(static_cast<bool>(address));
    auto endpoint = vix::network::Endpoint::create(address.take(), 0);
    require(static_cast<bool>(endpoint));
    auto listener_result = provider.listen(endpoint.value());
    require(static_cast<bool>(listener_result));
    auto listener = listener_result.take();
    auto local = listener.local_endpoint();
    require(static_cast<bool>(local));
    const auto peer = local.take();

    std::atomic<bool> server_exchange_ok{false};
    std::atomic<int> server_saw_no_http_after_auth_failure{0};
    std::thread server([&]
    {
      auto accept_op = vix::http::https_accept(provider, listener, tls_server);
      if (!accept_op) return;
      auto accepted = vix::network::run(provider, accept_op.take());
      if (!accepted || accepted.value().status() != vix::CompletionStatus::Success) return;
      auto session = accepted.take().take();
      const auto premature_response = session.send_response(vix::http::Response{});
      if (premature_response ||
          premature_response.error().identity() != vix::http::invalid_session_state)
        return;
      auto request_op = session.receive_request();
      if (!request_op) return;
      auto request_result = vix::network::run(provider, request_op.take());
      if (!request_result || request_result.value().status() != vix::CompletionStatus::Success)
        return;
      auto request = request_result.take().take();
      if (request.method != "POST" || request.target != "/secure" ||
          string_of(request.body) != "secret request")
        return;
      vix::http::Response response;
      response.status_code = 200;
      response.reason = "OK";
      response.headers.push_back({"Content-Type", "text/plain"});
      response.body = bytes("authenticated response");
      auto response_op = session.send_response(response);
      if (!response_op) return;
      response.body = bytes("mutated after operation start");
      auto sent = vix::network::run(provider, response_op.take());
      server_exchange_ok.store(
          sent && sent.value().status() == vix::CompletionStatus::Success,
          std::memory_order_release);

      for (int failure_case = 0; failure_case < 2; ++failure_case)
      {
        auto bad_accept_op = vix::http::https_accept(provider, listener, tls_server);
        if (!bad_accept_op) return;
        auto bad_accepted = vix::network::run(provider, bad_accept_op.take());
        if (!bad_accepted || bad_accepted.value().status() != vix::CompletionStatus::Success)
        {
          server_saw_no_http_after_auth_failure.fetch_add(1, std::memory_order_release);
          continue;
        }
        auto bad_session = bad_accepted.take().take();
        auto bad_request_op = bad_session.receive_request();
        if (!bad_request_op) return;
        auto bad_request = vix::network::run(provider, bad_request_op.take());
        if (!bad_request || bad_request.value().status() != vix::CompletionStatus::Success)
          server_saw_no_http_after_auth_failure.fetch_add(1, std::memory_order_release);
      }
    });

    vix::http::Request request;
    request.method = "POST";
    request.target = "/secure";
    request.headers.push_back({"Host", "localhost"});
    request.body = bytes("secret request");
    auto operation = vix::http::https_exchange(provider, peer, request, tls_client);
    require(static_cast<bool>(operation));
    request.body = bytes("mutated after operation creation");
    auto observed = vix::network::run(provider, operation.take());
    require(observed && observed.value().status() == vix::CompletionStatus::Success);
    const auto response = observed.take().take();
    require(response.status_code == 200);
    require(string_of(response.body) == "authenticated response");
    require(response.headers.size() >= 2);

    auto wrong_identity = client_config(credentials, "wrong.example");
    auto failed_operation = vix::http::https_exchange(provider, peer, request, wrong_identity);
    require(static_cast<bool>(failed_operation));
    auto failed = vix::network::run(provider, failed_operation.take());
    require(failed && failed.value().status() == vix::CompletionStatus::Failure);
    require(failed.value().error().identity() == vix::tls::peer_identity_mismatch);

    auto untrusted_config = system_trust_client_config();
    auto untrusted_operation = vix::http::https_exchange(
        provider, peer, request, untrusted_config);
    require(static_cast<bool>(untrusted_operation));
    auto untrusted = vix::network::run(provider, untrusted_operation.take());
    require(untrusted && untrusted.value().status() == vix::CompletionStatus::Failure);
    require(untrusted.value().error().identity() == vix::tls::authentication_failed);

    vix::CancellationSource cancellation;
    require(cancellation.request_cancel());
    vix::http::ExchangeOptions cancelled_options;
    cancelled_options.operation.cancellation = cancellation.token();
    auto cancelled_operation = vix::http::https_exchange(
        provider, peer, request, tls_client, cancelled_options);
    require(static_cast<bool>(cancelled_operation));
    auto cancelled = vix::network::run(provider, cancelled_operation.take());
    require(cancelled && cancelled.value().status() == vix::CompletionStatus::Cancelled);

    vix::http::ExchangeOptions expired_options;
    expired_options.operation.deadline = vix::Deadline::at(vix::MonotonicClock::now());
    auto expired_operation = vix::http::https_exchange(
        provider, peer, request, tls_client, expired_options);
    require(static_cast<bool>(expired_operation));
    auto expired = vix::network::run(provider, expired_operation.take());
    require(expired && expired.value().status() == vix::CompletionStatus::Failure);
    require(expired.value().error().identity() == vix::network::timed_out);
    server.join();
    require(server_exchange_ok.load(std::memory_order_acquire));
    require(server_saw_no_http_after_auth_failure.load(std::memory_order_acquire) == 2);
    require(static_cast<bool>(listener.close()));
    require(static_cast<bool>(provider.shutdown()));
  }

  void cooperative_https_exchange()
  {
    Credentials credentials;
    auto tls_server = server_config(credentials);
    auto tls_client = client_config(credentials, "localhost");
    std::mutex mutex;
    std::condition_variable ready;
    bool listener_ready = false;
    bool server_ok = false;
    std::uint16_t port{};

    std::thread server([&]
    {
      auto provider_result = vix::network::Provider::create_cooperative();
      require(static_cast<bool>(provider_result));
      auto provider = provider_result.take();
      auto address = vix::network::Address::parse("127.0.0.1");
      require(static_cast<bool>(address));
      auto endpoint = vix::network::Endpoint::create(address.take(), 0);
      require(static_cast<bool>(endpoint));
      auto listener_result = provider.listen(endpoint.value());
      require(static_cast<bool>(listener_result));
      auto listener = listener_result.take();
      auto local = listener.local_endpoint();
      require(static_cast<bool>(local));
      {
        std::lock_guard lock(mutex);
        port = local.value().port();
        listener_ready = true;
      }
      ready.notify_one();

      auto accept_op = vix::http::https_accept(provider, listener, tls_server);
      if (accept_op)
      {
        auto accepted = vix::network::run(provider, accept_op.take());
        if (accepted && accepted.value().status() == vix::CompletionStatus::Success)
        {
          auto session = accepted.take().take();
          auto request_op = session.receive_request();
          if (request_op)
          {
            auto received = vix::network::run(provider, request_op.take());
            if (received && received.value().status() == vix::CompletionStatus::Success)
            {
              auto request = received.take().take();
              vix::http::Response response;
              response.body = bytes("cooperative TLS");
              auto response_op = session.send_response(response);
              if (request.target == "/cooperative" && response_op)
              {
                auto sent = vix::network::run(provider, response_op.take());
                server_ok = sent &&
                            sent.value().status() == vix::CompletionStatus::Success;
              }
            }
          }
        }
      }
      (void)listener.close();
      require(static_cast<bool>(provider.shutdown()));
    });

    std::uint16_t bound_port{};
    {
      std::unique_lock lock(mutex);
      require(ready.wait_for(lock, std::chrono::seconds(10), [&] { return listener_ready; }));
      bound_port = port;
    }
    auto provider_result = vix::network::Provider::create_cooperative();
    require(static_cast<bool>(provider_result));
    auto provider = provider_result.take();
    auto address = vix::network::Address::parse("127.0.0.1");
    require(static_cast<bool>(address));
    auto endpoint = vix::network::Endpoint::create(address.take(), bound_port);
    require(static_cast<bool>(endpoint));
    vix::http::Request request;
    request.target = "/cooperative";
    request.headers.push_back({"Host", "localhost"});
    auto operation = vix::http::https_exchange(provider, endpoint.value(), request, tls_client);
    require(static_cast<bool>(operation));
    auto result = vix::network::run(provider, operation.take());
    require(result && result.value().status() == vix::CompletionStatus::Success);
    require(string_of(result.take().take().body) == "cooperative TLS");
    server.join();
    require(server_ok);
    require(static_cast<bool>(provider.shutdown()));
  }
}

int main()
{
  authenticated_exchange_and_identity_failure();
  cooperative_https_exchange();
}
