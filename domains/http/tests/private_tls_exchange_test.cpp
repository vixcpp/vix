#include "Http1.hpp"
#include "PrivateTlsExchange.hpp"
#include "HttpTlsAccess.hpp"
#include "CoroutineTask.hpp"
#include "Engine.hpp"

#include <vix/tls/TlsChannel.hpp>
#include <vix/tls/TlsConfig.hpp>

#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>
#include <vix/Deadline.hpp>
#include <vix/Duration.hpp>
#include <vix/MonotonicClock.hpp>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{
  using namespace vix::network;
  using namespace vix::http::detail;

  std::mutex progress_mutex;
  std::condition_variable progress_changed;
  unsigned next_scenario_id{};
  unsigned selected_scenario_id{};

  void notify_progress() noexcept
  {
    // Every background predicate wait holds progress_mutex while checking
    // state protected elsewhere. Publish a notification through the same
    // mutex so a state transition cannot land between the predicate check
    // and condition-variable wait registration.
    {
      std::lock_guard lock(progress_mutex);
    }
    progress_changed.notify_all();
  }

  struct ScenarioTrace
  {
    std::mutex mutex;
    unsigned id{};
    const char *client_phase{};
    const char *server_phase{};
    const char *harness_phase{};

    bool begin(const char *label)
    {
      std::lock_guard lock(mutex);
      id = ++next_scenario_id;
      if (selected_scenario_id != 0 && id != selected_scenario_id)
        return false;
      std::fprintf(stderr, "HTTPS CASE %u BEGIN %s\n", id, label);
      std::fflush(stderr);
      return true;
    }

    void mark(const char *side, const char *phase) noexcept
    {
      std::lock_guard lock(mutex);
      const char **previous = std::strcmp(side, "client") == 0
                                  ? &client_phase
                                  : (std::strcmp(side, "server") == 0
                                         ? &server_phase
                                         : &harness_phase);
      if (*previous && std::strcmp(*previous, phase) == 0)
        return;
      *previous = phase;
      std::fprintf(stderr, "HTTPS CASE %u %s: %s\n", id, side, phase);
      std::fflush(stderr);
    }
  };

  void trace_exchange_phase(void *context, const char *phase) noexcept
  {
    static_cast<ScenarioTrace *>(context)->mark("client", phase);
  }

  struct Credentials
  {
    EVP_PKEY *key{};
    X509 *certificate{};
    std::string certificate_path;
    std::string key_path;

    Credentials()
    {
      EVP_PKEY_CTX *key_context = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
      assert(key_context && EVP_PKEY_keygen_init(key_context) == 1);
      assert(EVP_PKEY_CTX_set_rsa_keygen_bits(key_context, 2048) == 1);
      assert(EVP_PKEY_keygen(key_context, &key) == 1);
      EVP_PKEY_CTX_free(key_context);

      certificate = X509_new();
      assert(certificate);
      assert(X509_set_version(certificate, 2) == 1);
      assert(ASN1_INTEGER_set(X509_get_serialNumber(certificate), 7) == 1);
      assert(X509_gmtime_adj(X509_getm_notBefore(certificate), -60));
      assert(X509_gmtime_adj(X509_getm_notAfter(certificate), 3600));
      assert(X509_set_pubkey(certificate, key) == 1);
      X509_NAME *name = X509_get_subject_name(certificate);
      constexpr unsigned char common_name[] = "localhost";
      assert(X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                        common_name, -1, -1, 0) == 1);
      assert(X509_set_issuer_name(certificate, name) == 1);
      X509V3_CTX extension_context{};
      X509V3_set_ctx(&extension_context, certificate, certificate,
                     nullptr, nullptr, 0);
      char basic_constraints_text[] = "critical,CA:TRUE";
      X509_EXTENSION *basic_constraints = X509V3_EXT_conf_nid(
          nullptr, &extension_context, NID_basic_constraints,
          basic_constraints_text);
      assert(basic_constraints && X509_add_ext(certificate,
                                               basic_constraints, -1) == 1);
      X509_EXTENSION_free(basic_constraints);
      char subject_alt_name_text[] = "DNS:localhost,IP:127.0.0.1";
      X509_EXTENSION *subject_alt_name = X509V3_EXT_conf_nid(
          nullptr, &extension_context, NID_subject_alt_name,
          subject_alt_name_text);
      assert(subject_alt_name && X509_add_ext(certificate,
                                              subject_alt_name, -1) == 1);
      X509_EXTENSION_free(subject_alt_name);
      assert(X509_sign(certificate, key, EVP_sha256()) > 0);

      char cert_template[] = "/tmp/vix-http-tls-cert-XXXXXX";
      int fd = ::mkstemp(cert_template);
      assert(fd >= 0);
      certificate_path = cert_template;
      FILE *cert_file = ::fdopen(fd, "w");
      assert(cert_file && PEM_write_X509(cert_file, certificate) == 1);
      assert(std::fclose(cert_file) == 0);

      char key_template[] = "/tmp/vix-http-tls-key-XXXXXX";
      fd = ::mkstemp(key_template);
      assert(fd >= 0);
      key_path = key_template;
      FILE *private_key_file = ::fdopen(fd, "w");
      assert(private_key_file &&
             PEM_write_PrivateKey(private_key_file, key, nullptr, nullptr,
                                  0, nullptr, nullptr) == 1);
      assert(std::fclose(private_key_file) == 0);
    }

    ~Credentials()
    {
      if (!certificate_path.empty())
        (void)::unlink(certificate_path.c_str());
      if (!key_path.empty())
        (void)::unlink(key_path.c_str());
      X509_free(certificate);
      EVP_PKEY_free(key);
    }
  };

  template <class T>
  struct Observation
  {
    std::mutex mutex;
    std::condition_variable changed;
    std::optional<vix::Completion<T>> completion;
    unsigned publications{};

    void publish(vix::Completion<T> value)
    {
      {
        std::lock_guard lock(mutex);
        assert(!completion);
        completion.emplace(std::move(value));
        ++publications;
      }
      changed.notify_all();
      notify_progress();
    }

    bool done()
    {
      std::lock_guard lock(mutex);
      return completion.has_value();
    }

    bool wait_for(std::chrono::seconds timeout)
    {
      std::unique_lock lock(mutex);
      return changed.wait_for(lock, timeout,
                              [&] { return completion.has_value(); });
    }

    vix::Completion<T> take()
    {
      std::lock_guard lock(mutex);
      assert(completion);
      return std::move(*completion);
    }
  };

  enum class HttpReply
  {
    normal,
    informational_coalesced,
    informational_limit_exceeded,
    informational_fragmented,
    informational_only,
    switching,
    head,
    connect,
    post
  };

  const char *reply_name(HttpReply reply) noexcept
  {
    switch (reply)
    {
    case HttpReply::normal: return "normal";
    case HttpReply::informational_coalesced: return "informational_coalesced";
    case HttpReply::informational_limit_exceeded: return "informational_limit_exceeded";
    case HttpReply::informational_fragmented: return "informational_fragmented";
    case HttpReply::informational_only: return "informational_only";
    case HttpReply::switching: return "switching";
    case HttpReply::head: return "head";
    case HttpReply::connect: return "connect";
    case HttpReply::post: return "post";
    }
    return "unknown";
  }

  struct Task
  {
    struct promise_type
    {
      std::shared_ptr<vix::network::test_detail::FinalSuspendState> final_suspend_state =
          std::make_shared<vix::network::test_detail::FinalSuspendState>();
      Task get_return_object()
      {
        return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
      }
      std::suspend_never initial_suspend() const noexcept { return {}; }
      vix::network::test_detail::FinalSuspendAwaiter<promise_type>
      final_suspend() const noexcept { return {}; }
      void return_void() const noexcept {}
      void unhandled_exception() const noexcept { std::terminate(); }
    };
    std::coroutine_handle<promise_type> handle;
    explicit Task(std::coroutine_handle<promise_type> value) : handle(value) {}
    Task(Task &&other) noexcept : handle(std::exchange(other.handle, {})) {}
    Task(const Task &) = delete;
    ~Task() { vix::network::test_detail::destroy_after_final_suspend(handle); }
  };

  template <class T>
  Task observe(vix::network::Operation<T> operation,
               const std::shared_ptr<Observation<T>> &result)
  {
    result->publish(co_await std::move(operation));
  }

  Task serve_tls(vix::tls::TlsChannel channel,
                 const std::shared_ptr<Observation<bool>> &result,
                 bool close_delimited_response,
                 bool truncated_response,
                 bool wait_for_client_termination,
                 const std::shared_ptr<std::atomic<bool>> &request_seen,
                 const std::shared_ptr<ScenarioTrace> &trace,
                 HttpReply reply = HttpReply::normal,
                 std::string expected_method = "GET")
  {
    trace->mark("server", "await TLS handshake");
    auto handshake = co_await channel.handshake();
    if (handshake.status() != vix::CompletionStatus::Success)
    {
      trace->mark("server", "TLS handshake failed; publish result");
      result->publish(vix::Completion<bool>{handshake.error()});
      co_return;
    }

    Http1Parser parser;
    std::array<std::byte, 13> input{};
    bool complete = false;
    trace->mark("server", "receive and parse HTTP request");
    for (unsigned count = 0; count < 256 && !complete; ++count)
    {
      auto step = parser.next(MessageKind::request, {}, {});
      if (step.status == ParseStatus::message_complete)
      {
        complete = true;
        break;
      }
      if (step.status == ParseStatus::protocol_error)
        break;
      auto received = co_await channel.receive(input);
      if (received.status() != vix::CompletionStatus::Success)
        break;
      const auto size = received.take();
      if (size == 0 || parser.append(
                            std::span<const std::byte>{input.data(), size}) != size)
        break;
      step = parser.next(MessageKind::request, {}, {});
      if (step.status == ParseStatus::headers_complete)
        step = parser.next(MessageKind::request, {}, {});
      complete = step.status == ParseStatus::message_complete;
      if (step.status == ParseStatus::protocol_error)
        break;
    }
    const auto expected_target = expected_method == "CONNECT"
                                    ? std::string_view{"localhost:443"}
                                    : std::string_view{"/"};
    if (!complete || parser.head().method != expected_method ||
        parser.head().target != expected_target)
    {
      trace->mark("server", "request parse failed; close channel");
      channel.close();
      result->publish(vix::Completion<bool>{false});
      co_return;
    }

    if (wait_for_client_termination)
    {
      trace->mark("server", "await client TLS termination after request");
      request_seen->store(true, std::memory_order_release);
      notify_progress();
      std::array<std::byte, 1> pending{};
      auto terminal = co_await channel.receive(pending);
      channel.close();
      trace->mark("server", "publish termination-wait result");
      if (terminal.status() == vix::CompletionStatus::Failure)
        result->publish(vix::Completion<bool>{terminal.error()});
      else
        result->publish(vix::Completion<bool>{false});
      co_return;
    }

    std::string response_head;
    if (reply == HttpReply::informational_coalesced ||
        reply == HttpReply::informational_limit_exceeded)
    {
      const auto count = max_informational_responses +
                         (reply == HttpReply::informational_limit_exceeded ? 1U : 0U);
      for (std::size_t i = 0; i < count; ++i)
        response_head += "HTTP/1.1 100 Continue\r\n\r\n";
      response_head += "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n"
                       "Connection: close\r\n\r\nOK";
    }
    else if (reply == HttpReply::informational_fragmented)
    {
      std::vector<std::byte> early;
      const std::string early_text = "HTTP/1.1 102 Processing\r\n\r\n";
      early.resize(early_text.size());
      std::transform(early_text.begin(), early_text.end(), early.begin(),
                     [](char value) { return static_cast<std::byte>(value); });
      auto early_sent = co_await channel.send(early);
      if (early_sent.status() != vix::CompletionStatus::Success)
      {
        result->publish(vix::Completion<bool>{early_sent.error()});
        co_return;
      }
      response_head = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
    }
    else if (reply == HttpReply::informational_only)
      response_head = "HTTP/1.1 100 Continue\r\n\r\n";
    else if (reply == HttpReply::switching)
      response_head = "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\n"
                      "Upgrade: example\r\n\r\n";
    else if (reply == HttpReply::connect)
      response_head = "HTTP/1.1 200 Connection Established\r\n\r\n";
    else
    {
      const std::array<HeaderField, 1> response_fields{{{"Connection", "close"}}};
      assert(serialize_response_head(
          200, "OK", response_fields,
          close_delimited_response
              ? OutgoingFraming{OutgoingFraming::Kind::no_body, 0}
              : OutgoingFraming{OutgoingFraming::Kind::fixed_length, 2},
          response_head));
      if (reply != HttpReply::head)
        response_head += truncated_response ? "O" : "OK";
    }
    std::vector<std::byte> response(response_head.size());
    std::transform(response_head.begin(), response_head.end(), response.begin(),
                   [](char value) { return static_cast<std::byte>(value); });
    trace->mark("server", "await TLS response send");
    auto sent = co_await channel.send(response);
    if (sent.status() != vix::CompletionStatus::Success)
    {
      trace->mark("server", "TLS response send failed");
      result->publish(vix::Completion<bool>{sent.error()});
      co_return;
    }
    if (reply == HttpReply::informational_only || reply == HttpReply::switching ||
        reply == HttpReply::connect)
    {
      channel.close();
      result->publish(vix::Completion<bool>{true});
      co_return;
    }
    if (truncated_response)
    {
      auto shutdown = co_await channel.shutdown();
      if (shutdown.status() == vix::CompletionStatus::Failure)
        result->publish(vix::Completion<bool>{shutdown.error()});
      else
        result->publish(vix::Completion<bool>{false});
      co_return;
    }
    // The HTTP client closes after consuming the fixed-length response. Keep
    // the TLS peer alive until that authenticated close_notify arrives, then
    // complete the reciprocal close_notify exchange.
    std::array<std::byte, 1> after_response{};
    trace->mark("server", "await client close_notify");
    auto peer_close = co_await channel.receive(after_response);
    if (peer_close.status() != vix::CompletionStatus::Failure ||
        peer_close.error().identity() != vix::tls::peer_closed)
    {
      trace->mark("server", "client close_notify wait ended unexpectedly");
      channel.close();
      if (peer_close.status() == vix::CompletionStatus::Failure)
        result->publish(vix::Completion<bool>{peer_close.error()});
      else
        result->publish(vix::Completion<bool>{false});
      co_return;
    }
    trace->mark("server", "await reciprocal TLS close_notify");
    auto shutdown = co_await channel.shutdown();
    trace->mark("server", "publish normal result");
    if (shutdown.status() != vix::CompletionStatus::Success)
      result->publish(vix::Completion<bool>{shutdown.error()});
    else
      result->publish(vix::Completion<bool>{true});
  }

  bool progress_until(Provider &provider, bool cooperative,
                      const auto &predicate,
                      const std::shared_ptr<ScenarioTrace> &trace,
                      const char *wait_phase)
  {
    trace->mark("harness", wait_phase);
    if (cooperative)
    {
      const auto limit = std::chrono::steady_clock::now() +
                         std::chrono::seconds(15);
      while (!predicate() && std::chrono::steady_clock::now() < limit)
      {
        assert(provider.poll());
        std::this_thread::yield();
      }
      const bool completed = predicate();
      if (!completed)
        trace->mark("harness", "cooperative progress wait expired");
      return completed;
    }
    std::unique_lock lock(progress_mutex);
    const bool completed = progress_changed.wait_for(
        lock, std::chrono::seconds(15), predicate);
    if (!completed)
      trace->mark("harness", "background observation wait expired");
    return completed;
  }

  bool run_exchange(bool cooperative, bool identity_matches,
                    bool close_delimited_response = false,
                    bool cancel_before_handshake = false,
                    bool expire_before_handshake = false,
                    bool cancel_during_receive = false,
                    bool expire_during_receive = false,
                    bool truncated_response = false,
                    HttpReply reply = HttpReply::normal,
                    std::string request_method = "GET",
                    bool default_deadline_during_receive = false)
  {
    auto trace = std::make_shared<ScenarioTrace>();
    char scenario_description[256]{};
    std::snprintf(scenario_description, sizeof(scenario_description),
                  "mode=%s reply=%s method=%s identity=%s cancel_before=%d deadline_before=%d cancel_receive=%d deadline_receive=%d default_deadline=%d truncated=%d close_delimited=%d",
                  cooperative ? "cooperative" : "background", reply_name(reply),
                  request_method.c_str(), identity_matches ? "match" : "mismatch",
                  cancel_before_handshake, expire_before_handshake,
                  cancel_during_receive, expire_during_receive,
                  default_deadline_during_receive, truncated_response,
                  close_delimited_response);
    if (!trace->begin(scenario_description))
      return true;
    trace->mark("harness", "create TLS configuration and Network provider");
    static Credentials credentials;
    vix::tls::TlsClientOptions client_options;
    client_options.expected_peer = {
        vix::tls::PeerIdentityKind::dns_name,
        identity_matches ? "localhost" : "wrong.invalid"};
    client_options.use_system_trust = false;
    client_options.ca_file = credentials.certificate_path;
    auto client_config = vix::tls::TlsClientConfig::create(
        std::move(client_options));
    assert(client_config);
    vix::tls::TlsServerOptions server_options;
    server_options.certificate_chain_file = credentials.certificate_path;
    server_options.private_key_file = credentials.key_path;
    auto server_config = vix::tls::TlsServerConfig::create(
        std::move(server_options));
    assert(server_config);

    auto provider_result = cooperative ? Provider::create_cooperative()
                                       : Provider::create();
    assert(provider_result);
    auto provider = provider_result.take();
    auto address = Address::parse("127.0.0.1");
    assert(address);
    auto endpoint = Endpoint::create(address.take(), 0);
    assert(endpoint);
    auto listener_result = provider.listen(endpoint.take());
    assert(listener_result);
    auto listener = listener_result.take();
    auto local_endpoint = listener.local_endpoint();
    assert(local_endpoint);

    auto accepted = std::make_shared<Observation<TcpConnection>>();
    auto connected = std::make_shared<Observation<TcpConnection>>();
    [[maybe_unused]] auto accept_task = observe(listener.accept(), accepted);
    [[maybe_unused]] auto connect_task = observe(
        provider.connect(local_endpoint.value()), connected);
    trace->mark("harness", "await TCP accept/connect");
    assert(progress_until(provider, cooperative,
                          [&] { return accepted->done() && connected->done(); },
                          trace, "wait for TCP accept/connect"));
    trace->mark("harness", "TCP accept/connect completed");
    auto accepted_completion = accepted->take();
    auto connected_completion = connected->take();
    assert(accepted_completion.status() == vix::CompletionStatus::Success);
    assert(connected_completion.status() == vix::CompletionStatus::Success);

    auto server_result = vix::tls::TlsChannel::create_server(
        accepted_completion.take(), server_config.value());
    auto client_connection = connected_completion.take();
    std::optional<TcpConnection> alias{
        vix::network::detail::NetworkAccess::alias_tcp_for_test(client_connection)};
    vix::network::detail::NetworkAccess::limit_send_chunk_for_test(*alias, 7);
    auto client_result = vix::tls::TlsChannel::create_client(
        std::move(client_connection), client_config.value());
    assert(server_result && client_result);
    auto server_channel = server_result.take();
    auto client_channel = client_result.take();

    // TLS owns the one TCP lease. An alias created before ownership transfer
    // cannot start an ordinary Network receive or reach native I/O afterward.
    const auto counters_before =
        vix::network::detail::NetworkAccess::transport_counters_for_test(*alias);
    std::array<std::byte, 1> forbidden_buffer{};
    auto forbidden_observation =
        std::make_shared<Observation<std::size_t>>();
    [[maybe_unused]] auto forbidden_task = observe(
        alias->receive(forbidden_buffer), forbidden_observation);
    assert(progress_until(provider, cooperative,
                          [&] { return forbidden_observation->done(); },
                          trace, "wait for rejected alias operation"));
    auto forbidden = forbidden_observation->take();
    assert(forbidden.status() == vix::CompletionStatus::Failure);
    assert(forbidden.error().identity() == vix::network::transport_owned);
    const auto counters_after =
        vix::network::detail::NetworkAccess::transport_counters_for_test(*alias);
    assert(counters_after.native_recv_attempts ==
           counters_before.native_recv_attempts);

    auto server_observation = std::make_shared<Observation<bool>>();
    auto request_seen = std::make_shared<std::atomic<bool>>();
    trace->mark("harness", "start TLS server coroutine");
    [[maybe_unused]] auto server_task = serve_tls(
        std::move(server_channel), server_observation,
        close_delimited_response, truncated_response,
        cancel_during_receive || expire_during_receive ||
            default_deadline_during_receive,
        request_seen, trace, reply, request_method);
    std::string request;
    const std::array<HeaderField, 2> request_fields{{
        {"Host", "localhost"}, {"Connection", "close"}}};
    const auto request_target = request_method == "CONNECT"
                                    ? std::string_view{"localhost:443"}
                                    : std::string_view{"/"};
    assert(serialize_request_head(request_method, request_target, request_fields,
                                  {OutgoingFraming::Kind::no_body, 0}, request));
    std::vector<std::byte> request_bytes(request.size());
    std::transform(request.begin(), request.end(), request_bytes.begin(),
                   [](char value) { return static_cast<std::byte>(value); });
    vix::CancellationSource cancellation;
    vix::network::OperationOptions options;
    if (cancel_before_handshake || cancel_during_receive)
    {
      options.cancellation = cancellation.token();
      if (cancel_before_handshake)
        assert(cancellation.request_cancel());
    }
    if (expire_before_handshake)
      options.deadline = vix::Deadline::at(vix::MonotonicClock::now());
    else if (expire_during_receive)
      options.deadline = vix::Deadline::after(vix::Duration::milliseconds(200));
    trace->mark("harness", "create HTTPS parent operation");
    auto exchange = start_private_tls_exchange(
        std::move(client_channel), request_bytes, std::move(options),
        default_deadline_during_receive
            ? vix::Duration::seconds(2)
            : default_exchange_lifetime,
        (reply == HttpReply::informational_coalesced ||
         reply == HttpReply::informational_limit_exceeded)
            ? max_exchange_receive_chunk
            : default_exchange_receive_chunk,
        PrivateExchangeTestObserver{trace_exchange_phase, trace.get()});
    assert(exchange);
    auto client_observation = std::make_shared<Observation<ExchangeResponse>>();
    [[maybe_unused]] auto client_task = observe(exchange.take(), client_observation);

    if (cancel_during_receive || default_deadline_during_receive)
    {
      trace->mark("harness", "await server request before cancellation/deadline");
      assert(progress_until(provider, cooperative, [&]
                            { return request_seen->load(std::memory_order_acquire); },
                            trace, "wait for server to receive request"));
      assert(!client_observation->done());
      if (cancel_during_receive)
        assert(cancellation.request_cancel());
    }

    trace->mark("harness", "await client and server terminal completions");
    assert(progress_until(provider, cooperative,
                          [&] { return client_observation->done() &&
                                       server_observation->done(); },
                          trace, "wait for client and server completion"));
    trace->mark("harness", "both terminal completions observed");
    auto client_completion = client_observation->take();
    auto server_completion = server_observation->take();
    assert(client_observation->publications == 1);
    assert(server_observation->publications == 1);

    if (!identity_matches)
    {
      assert(client_completion.status() == vix::CompletionStatus::Failure);
      assert(server_completion.status() != vix::CompletionStatus::Success);
    }
    else if (cancel_before_handshake || cancel_during_receive)
    {
      assert(client_completion.status() == vix::CompletionStatus::Cancelled);
      assert(server_completion.status() != vix::CompletionStatus::Success);
    }
    else if (expire_before_handshake || expire_during_receive ||
             default_deadline_during_receive)
    {
      assert(client_completion.status() == vix::CompletionStatus::Failure);
      assert(client_completion.error().identity() == vix::network::timed_out);
      assert(server_completion.status() != vix::CompletionStatus::Success);
    }
    else if (truncated_response)
    {
      assert(client_completion.status() == vix::CompletionStatus::Failure);
      assert(client_completion.error().identity() == malformed_response);
      assert(server_completion.status() != vix::CompletionStatus::Success);
    }
    else if (close_delimited_response)
    {
      assert(client_completion.status() == vix::CompletionStatus::Failure);
      assert(client_completion.error().identity() ==
             unsupported_response_framing);
      assert(server_completion.status() != vix::CompletionStatus::Success);
    }
    else if (reply == HttpReply::informational_limit_exceeded)
    {
      assert(client_completion.status() == vix::CompletionStatus::Failure);
      assert(client_completion.error().identity() ==
             informational_response_limit_exceeded);
      assert(server_completion.status() != vix::CompletionStatus::Success);
    }
    else if (reply == HttpReply::switching || reply == HttpReply::connect ||
             reply == HttpReply::informational_only)
    {
      assert(client_completion.status() == vix::CompletionStatus::Failure);
      if (reply == HttpReply::informational_only)
        assert(client_completion.error().identity() == malformed_response ||
               client_completion.error().identity() == vix::tls::peer_truncated);
      else
        assert(client_completion.error().identity() == malformed_response);
    }
    else
    {
      if (client_completion.status() != vix::CompletionStatus::Success ||
          server_completion.status() != vix::CompletionStatus::Success)
      {
        const auto print_failure = [](const char *side, const auto &completion)
        {
          std::fprintf(stderr, "HTTPS %s completion status=%u", side,
                       static_cast<unsigned>(completion.status()));
          if (completion.status() == vix::CompletionStatus::Failure)
            std::fprintf(stderr, " error=%.*s:%u message=%.*s",
                         static_cast<int>(completion.error().identity().domain().size()),
                         completion.error().identity().domain().data(),
                         completion.error().identity().code(),
                         static_cast<int>(completion.error().message().size()),
                         completion.error().message().data());
          std::fputc('\n', stderr);
        };
        std::fprintf(stderr, "HTTPS provider mode=%s\n",
                     cooperative ? "cooperative" : "background");
        print_failure("client", client_completion);
        print_failure("server", server_completion);
      }
      assert(client_completion.status() == vix::CompletionStatus::Success);
      assert(server_completion.status() == vix::CompletionStatus::Success);
      auto response = client_completion.take();
      assert(response.status_code == 200);
      const bool no_body = reply == HttpReply::head;
      assert(response.body.size() == (no_body ? 0U : 2U));
      if (!no_body)
        assert(response.body[0] == std::byte{'O'} &&
               response.body[1] == std::byte{'K'});
      const auto counters =
          vix::network::detail::NetworkAccess::transport_counters_for_test(*alias);
      assert(counters.native_send_attempts > 1);
      assert(counters.native_recv_attempts > 1);
    }

    (void)listener.close();
    trace->mark("harness", "retire provider resources and operations");
    for (unsigned attempt = 0; attempt < 10000 &&
         vix::network::detail::NetworkAccess::core_for_test(provider)
                 ->active_resources_for_test() != 0;
         ++attempt)
    {
      if (cooperative)
        assert(provider.poll());
      else
        std::this_thread::yield();
    }
    auto core = vix::network::detail::NetworkAccess::core_for_test(provider);
    assert(core->active_operations_for_test() == 0);
    assert(core->active_resources_for_test() == 0);
    alias.reset();
    trace->mark("harness", "shutdown Network provider");
    assert(provider.shutdown());
    trace->mark("harness", "scenario complete");
    return true;
  }
}

int main()
{
  if (const char *selected = std::getenv("VIX_HTTP_TLS_CASE"))
    selected_scenario_id = static_cast<unsigned>(std::strtoul(selected, nullptr, 10));
  assert(run_exchange(false, true));
  assert(run_exchange(true, true));
  assert(run_exchange(true, false));
  assert(run_exchange(true, true, true));
  assert(run_exchange(true, true, false, true));
  assert(run_exchange(true, true, false, false, true));
  assert(run_exchange(true, true, false, false, false, true));
  assert(run_exchange(true, true, false, false, false, false, true));
  assert(run_exchange(true, true, false, false, false, false, false, false,
                      HttpReply::normal, "GET", true));
  assert(run_exchange(true, true, false, false, false, false, false, true));
  assert(run_exchange(false, true, false, false, false, false, false, false,
                      HttpReply::informational_coalesced));
  assert(run_exchange(true, true, false, false, false, false, false, false,
                      HttpReply::informational_coalesced));
  assert(run_exchange(true, true, false, false, false, false, false, false,
                      HttpReply::informational_limit_exceeded));
  assert(run_exchange(false, true, false, false, false, false, false, false,
                      HttpReply::informational_fragmented));
  assert(run_exchange(true, true, false, false, false, false, false, false,
                      HttpReply::informational_fragmented));
  assert(run_exchange(false, true, false, false, false, false, false, false,
                      HttpReply::informational_only));
  assert(run_exchange(true, true, false, false, false, false, false, false,
                      HttpReply::informational_only));
  assert(run_exchange(false, true, false, false, false, false, false, false,
                      HttpReply::switching));
  assert(run_exchange(true, true, false, false, false, false, false, false,
                      HttpReply::switching));
  assert(run_exchange(false, true, false, false, false, false, false, false,
                      HttpReply::head, "HEAD"));
  assert(run_exchange(true, true, false, false, false, false, false, false,
                      HttpReply::head, "HEAD"));
  assert(run_exchange(false, true, false, false, false, false, false, false,
                      HttpReply::connect, "CONNECT"));
  assert(run_exchange(true, true, false, false, false, false, false, false,
                      HttpReply::connect, "CONNECT"));
  assert(run_exchange(false, true, false, false, false, false, false, false,
                      HttpReply::post, "POST"));
  assert(run_exchange(true, true, false, false, false, false, false, false,
                      HttpReply::post, "POST"));
}
