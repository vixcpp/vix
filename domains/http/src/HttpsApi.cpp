#include <vix/http/Https.hpp>

#include "Engine.hpp"
#include "Http1.hpp"
#include "HttpApiInternal.hpp"
#include "PrivateExchange.hpp"
#include "PrivateTlsExchange.hpp"

#include <vix/MonotonicClock.hpp>
#include <vix/network/Errors.hpp>
#include <vix/tls/Errors.hpp>
#include <vix/tls/TlsChannel.hpp>

#include <algorithm>
#include <array>
#include <coroutine>
#include <exception>
#include <mutex>
#include <utility>

namespace vix::http::detail
{
  class HttpsSessionState final
  {
  public:
    HttpsSessionState(vix::tls::TlsChannel owned_channel,
                      std::shared_ptr<vix::network::detail::ProviderCore> owned_provider,
                      vix::http::Limits configured_limits,
                      vix::network::OperationOptions default_options)
        : channel(std::move(owned_channel)), provider(std::move(owned_provider)),
          limits(configured_limits), options(std::move(default_options)) {}

    vix::tls::TlsChannel channel;
    std::shared_ptr<vix::network::detail::ProviderCore> provider;
    vix::http::Limits limits;
    vix::network::OperationOptions options;
    std::mutex mutex;
    bool request_started{};
    bool request_received{};
    bool response_started{};
    bool closed{};
    std::string request_method;
  };

  struct HttpsSessionAccess
  {
    static vix::http::HttpsServerSession make(
        std::shared_ptr<HttpsSessionState> state)
    { return vix::http::HttpsServerSession{std::move(state)}; }
  };

  namespace
  {
    constexpr auto exchange_lifetime = vix::Duration::seconds(30);
    constexpr std::size_t maximum_body = 65536;

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

    struct ProviderHandoff
    {
      std::shared_ptr<vix::network::detail::ProviderCore> provider;
      bool queued{true};
      bool await_ready() const noexcept { return false; }
      bool await_suspend(std::coroutine_handle<> continuation)
      {
        if (provider && provider->post([continuation] { continuation.resume(); }))
          return true;
        queued = false;
        return false;
      }
      bool await_resume() const noexcept { return queued; }
    };

    template <class T>
    void complete(const std::shared_ptr<vix::network::detail::OperationRecord<T>> &parent,
                  vix::Completion<T> result)
    {
      parent->complete(std::move(result));
    }

    void close_session(const std::shared_ptr<HttpsSessionState> &state) noexcept
    {
      {
        std::lock_guard lock(state->mutex);
        if (state->closed)
          return;
        state->closed = true;
      }
      state->channel.close();
    }

    struct ClientState
    {
      ClientState(vix::network::Operation<vix::network::TcpConnection> connect_operation,
                  std::vector<std::byte> request_bytes,
                  std::shared_ptr<vix::network::detail::ProviderCore> owner,
                  vix::tls::TlsClientConfig tls_config,
                  vix::network::OperationOptions operation_options,
                  std::size_t body_limit)
          : connect(std::move(connect_operation)), request(std::move(request_bytes)),
            provider(std::move(owner)), tls(std::move(tls_config)),
            options(std::move(operation_options)), response_limit(body_limit) {}

      vix::network::Operation<vix::network::TcpConnection> connect;
      std::vector<std::byte> request;
      std::shared_ptr<vix::network::detail::ProviderCore> provider;
      vix::tls::TlsClientConfig tls;
      vix::network::OperationOptions options;
      std::size_t response_limit{};
    };

    Detached drive_https_client(
        std::shared_ptr<ClientState> state,
        std::shared_ptr<vix::network::detail::OperationRecord<vix::http::Response>> parent)
    {
      try
      {
        auto connected = co_await std::move(state->connect);
        if (connected.status() != vix::CompletionStatus::Success)
        {
          if (connected.status() == vix::CompletionStatus::Cancelled)
            complete(parent, vix::Completion<vix::http::Response>::cancelled());
          else
            complete(parent, vix::Completion<vix::http::Response>{connected.error()});
          co_return;
        }

        auto channel = vix::tls::TlsChannel::create_client(connected.take(), state->tls);
        if (!channel)
        {
          complete(parent, vix::Completion<vix::http::Response>{channel.error()});
          co_return;
        }
        if (!(co_await ProviderHandoff{state->provider}))
        {
          complete(parent, vix::Completion<vix::http::Response>{vix::Error{
              vix::network::provider_stopped,
              "Network provider stopped before HTTPS handshake"}});
          co_return;
        }

        auto exchange = vix::http::detail::start_private_tls_exchange(
            channel.take(), state->request, state->options,
            vix::http::detail::default_exchange_lifetime,
            vix::http::detail::default_exchange_receive_chunk, {}, false,
            state->response_limit);
        if (!exchange)
        {
          complete(parent, vix::Completion<vix::http::Response>{exchange.error()});
          co_return;
        }
        auto terminal = co_await exchange.take();
        if (terminal.status() != vix::CompletionStatus::Success)
        {
          if (terminal.status() == vix::CompletionStatus::Cancelled)
            complete(parent, vix::Completion<vix::http::Response>::cancelled());
          else
            complete(parent, vix::Completion<vix::http::Response>{terminal.error()});
          co_return;
        }
        auto value = terminal.take();
        vix::http::Response response;
        response.version = std::move(value.version);
        response.status_code = value.status_code;
        response.reason = std::move(value.reason);
        response.headers = vix::http::detail::public_headers(value.headers);
        response.body = std::move(value.body);
        response.trailers = vix::http::detail::public_headers(value.trailers);
        complete(parent, vix::Completion<vix::http::Response>{std::move(response)});
      }
      catch (...)
      {
        complete(parent, vix::Completion<vix::http::Response>{vix::Error{
            vix::network::io_failure, "exception while progressing HTTPS exchange"}});
      }
    }

    struct AcceptState
    {
      AcceptState(vix::network::Operation<vix::network::TcpConnection> accept_operation,
                  std::shared_ptr<vix::network::detail::ProviderCore> owner,
                  vix::tls::TlsServerConfig server_config,
                  vix::http::Limits configured_limits,
                  vix::network::OperationOptions operation_options)
          : child(std::move(accept_operation)), provider(std::move(owner)),
            config(std::move(server_config)), limits(configured_limits),
            options(std::move(operation_options)) {}
      vix::network::Operation<vix::network::TcpConnection> child;
      std::shared_ptr<vix::network::detail::ProviderCore> provider;
      vix::tls::TlsServerConfig config;
      vix::http::Limits limits;
      vix::network::OperationOptions options;
    };

    Detached drive_https_accept(
        std::shared_ptr<AcceptState> state,
        std::shared_ptr<vix::network::detail::OperationRecord<
            vix::http::HttpsServerSession>> parent)
    {
      try
      {
        auto accepted = co_await std::move(state->child);
        if (accepted.status() != vix::CompletionStatus::Success)
        {
          if (accepted.status() == vix::CompletionStatus::Cancelled)
            complete(parent, vix::Completion<vix::http::HttpsServerSession>::cancelled());
          else
            complete(parent, vix::Completion<vix::http::HttpsServerSession>{accepted.error()});
          co_return;
        }
        auto channel = vix::tls::TlsChannel::create_server(accepted.take(), state->config);
        if (!channel)
        {
          complete(parent, vix::Completion<vix::http::HttpsServerSession>{channel.error()});
          co_return;
        }
        if (!(co_await ProviderHandoff{state->provider}))
        {
          complete(parent, vix::Completion<vix::http::HttpsServerSession>{vix::Error{
              vix::network::provider_stopped,
              "Network provider stopped before HTTPS server handshake"}});
          co_return;
        }
        auto handshake = co_await channel.value().handshake(state->options);
        if (handshake.status() != vix::CompletionStatus::Success)
        {
          if (handshake.status() == vix::CompletionStatus::Cancelled)
            complete(parent, vix::Completion<vix::http::HttpsServerSession>::cancelled());
          else
            complete(parent, vix::Completion<vix::http::HttpsServerSession>{handshake.error()});
          co_return;
        }
        auto session = std::make_shared<HttpsSessionState>(
            channel.take(), state->provider, state->limits, state->options);
        complete(parent, vix::Completion<vix::http::HttpsServerSession>{
            HttpsSessionAccess::make(std::move(session))});
      }
      catch (...)
      {
        complete(parent, vix::Completion<vix::http::HttpsServerSession>{vix::Error{
            vix::network::io_failure, "exception while accepting HTTPS session"}});
      }
    }

    struct RequestState
    {
      RequestState(std::shared_ptr<HttpsSessionState> owner,
                   vix::network::OperationOptions operation_options)
          : session(std::move(owner)), options(std::move(operation_options)) {}
      std::shared_ptr<HttpsSessionState> session;
      vix::network::OperationOptions options;
      vix::http::detail::Http1Parser parser;
      std::array<std::byte, 4096> input{};
      std::array<std::byte, 4096> body_chunk{};
      std::vector<std::byte> body;
    };

    Detached drive_https_request(
        std::shared_ptr<RequestState> state,
        std::shared_ptr<vix::network::detail::OperationRecord<vix::http::Request>> parent)
    {
      const auto session = state->session;
      {
        std::lock_guard lock(session->mutex);
        if (session->closed || session->request_started)
        {
          complete(parent, vix::Completion<vix::http::Request>{vix::Error{
              invalid_session_state, "HTTPS session cannot receive another request"}});
          co_return;
        }
        session->request_started = true;
      }
      try
      {
        for (;;)
        {
          const auto limit = std::min(session->limits.request_body_bytes, maximum_body);
          const auto capacity = std::min(state->body_chunk.size(), limit -
              std::min(state->body.size(), limit));
          auto step = state->parser.next(
              MessageKind::request, {},
              std::span<std::byte>{state->body_chunk}.first(capacity), false);
          if (step.status == ParseStatus::headers_complete)
          {
            if (state->parser.head().body_mode == BodyMode::fixed_length &&
                state->parser.head().content_length > limit)
            {
              close_session(session);
              complete(parent, vix::Completion<vix::http::Request>{vix::Error{
                  message_too_large, "HTTPS request body exceeds configured limit"}});
              co_return;
            }
            continue;
          }
          if (step.status == ParseStatus::body_data)
          {
            if (step.body_bytes == 0)
            {
              close_session(session);
              complete(parent, vix::Completion<vix::http::Request>{vix::Error{
                  message_too_large, "HTTPS request body exceeds configured limit"}});
              co_return;
            }
            state->body.insert(state->body.end(), state->body_chunk.begin(),
                               state->body_chunk.begin() +
                                   static_cast<std::ptrdiff_t>(step.body_bytes));
            continue;
          }
          if (step.status == ParseStatus::protocol_error)
          {
            close_session(session);
            complete(parent, vix::Completion<vix::http::Request>{vix::Error{
                invalid_message, "malformed or ambiguous HTTPS request"}});
            co_return;
          }
          if (step.status == ParseStatus::message_complete)
          {
            if (state->parser.buffered_bytes() != 0)
            {
              close_session(session);
              complete(parent, vix::Completion<vix::http::Request>{vix::Error{
                  invalid_message, "surplus bytes follow one-request HTTPS session message"}});
              co_return;
            }
            vix::http::Request request;
            request.method = state->parser.head().method;
            request.target = state->parser.head().target;
            request.version = state->parser.head().version;
            request.headers = public_headers(state->parser.head().fields);
            request.trailers = public_headers(state->parser.head().trailers);
            request.body = std::move(state->body);
            {
              std::lock_guard lock(session->mutex);
              session->request_received = true;
              session->request_method = request.method;
            }
            complete(parent, vix::Completion<vix::http::Request>{std::move(request)});
            co_return;
          }
          if (!(co_await ProviderHandoff{session->provider}))
          {
            close_session(session);
            complete(parent, vix::Completion<vix::http::Request>{vix::Error{
                vix::network::provider_stopped,
                "Network provider stopped during HTTPS request"}});
            co_return;
          }
          auto received = co_await session->channel.receive(state->input, state->options);
          if (received.status() == vix::CompletionStatus::Failure &&
              received.error().identity() == vix::tls::peer_closed)
          {
            close_session(session);
            complete(parent, vix::Completion<vix::http::Request>{vix::Error{
                invalid_message, "TLS closed before the HTTP request completed"}});
            co_return;
          }
          if (received.status() != vix::CompletionStatus::Success)
          {
            close_session(session);
            if (received.status() == vix::CompletionStatus::Cancelled)
              complete(parent, vix::Completion<vix::http::Request>::cancelled());
            else
              complete(parent, vix::Completion<vix::http::Request>{received.error()});
            co_return;
          }
          const auto count = received.take();
          if (count == 0 || state->parser.append(
                  std::span<const std::byte>{state->input.data(), count}) != count)
          {
            close_session(session);
            complete(parent, vix::Completion<vix::http::Request>{vix::Error{
                invalid_message, "HTTPS request exceeds parser staging limit"}});
            co_return;
          }
        }
      }
      catch (...)
      {
        close_session(session);
        complete(parent, vix::Completion<vix::http::Request>{vix::Error{
            vix::network::resource_exhausted,
            "exception while parsing HTTPS request"}});
      }
    }

    struct ResponseState
    {
      ResponseState(std::shared_ptr<HttpsSessionState> owner,
                    std::vector<std::byte> serialized_bytes, unsigned status_code,
                    vix::network::OperationOptions operation_options)
          : session(std::move(owner)), bytes(std::move(serialized_bytes)),
            status(status_code), options(std::move(operation_options)) {}
      std::shared_ptr<HttpsSessionState> session;
      std::vector<std::byte> bytes;
      unsigned status{};
      vix::network::OperationOptions options;
    };

    Detached drive_https_response(
        std::shared_ptr<ResponseState> state,
        std::shared_ptr<vix::network::detail::OperationRecord<void>> parent)
    {
      const auto session = state->session;
      bool unsupported_connect = false;
      {
        std::lock_guard lock(session->mutex);
        if (session->closed || !session->request_received || session->response_started)
        {
          complete(parent, vix::Completion<void>{vix::Error{
              invalid_session_state, "HTTPS response requires one received request"}});
          co_return;
        }
        if (session->request_method == "CONNECT" &&
            state->status >= 200 && state->status < 300)
        {
          unsupported_connect = true;
        }
        else
          session->response_started = true;
      }
      if (unsupported_connect)
      {
        close_session(session);
        complete(parent, vix::Completion<void>{vix::Error{
            unsupported_message, "CONNECT tunneling is not supported"}});
        co_return;
      }
      try
      {
        if (!(co_await ProviderHandoff{session->provider}))
        {
          close_session(session);
          complete(parent, vix::Completion<void>{vix::Error{
              vix::network::provider_stopped,
              "Network provider stopped before HTTPS response"}});
          co_return;
        }
        auto sent = co_await session->channel.send(state->bytes, state->options);
        if (sent.status() != vix::CompletionStatus::Success)
        {
          close_session(session);
          if (sent.status() == vix::CompletionStatus::Cancelled)
            complete(parent, vix::Completion<void>::cancelled());
          else
            complete(parent, vix::Completion<void>{sent.error()});
          co_return;
        }
        if (!(co_await ProviderHandoff{session->provider}))
        {
          close_session(session);
          complete(parent, vix::Completion<void>{vix::Error{
              vix::network::provider_stopped,
              "Network provider stopped before HTTPS close_notify"}});
          co_return;
        }
        auto shutdown = co_await session->channel.shutdown(state->options);
        close_session(session);
        if (shutdown.status() == vix::CompletionStatus::Success)
          complete(parent, vix::Completion<void>::success());
        else if (shutdown.status() == vix::CompletionStatus::Cancelled)
          complete(parent, vix::Completion<void>::cancelled());
        else
          complete(parent, vix::Completion<void>{shutdown.error()});
      }
      catch (...)
      {
        close_session(session);
        complete(parent, vix::Completion<void>{vix::Error{
            vix::network::io_failure,
            "exception while sending HTTPS response"}});
      }
    }
  }
}

namespace vix::http
{
  HttpsServerSession::HttpsServerSession(
      std::shared_ptr<detail::HttpsSessionState> state) noexcept
      : state_(std::move(state)) {}
  HttpsServerSession::HttpsServerSession(HttpsServerSession &&) noexcept = default;
  HttpsServerSession &HttpsServerSession::operator=(HttpsServerSession &&) noexcept = default;
  HttpsServerSession::~HttpsServerSession() { close(); }

  void HttpsServerSession::close() noexcept
  {
    if (state_)
      detail::close_session(state_);
  }

  vix::Result<vix::network::Operation<Request>>
  HttpsServerSession::receive_request(vix::network::OperationOptions options)
  {
    if (!state_)
      return vix::Error{invalid_session_state, "HTTPS session is empty"};
    options.deadline = vix::Deadline::earliest(options.deadline, state_->options.deadline);
    try
    {
      auto state = std::make_shared<detail::RequestState>(state_, options);
      auto operation = vix::network::detail::OperationFactory<Request>::make(
          state_->provider, [state](auto parent)
          { (void)detail::drive_https_request(state, parent); });
      return operation;
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate HTTPS request operation"};
    }
  }

  vix::Result<vix::network::Operation<void>>
  HttpsServerSession::send_response(const Response &response,
                                    vix::network::OperationOptions options)
  {
    if (!state_)
      return vix::Error{invalid_session_state, "HTTPS session is empty"};
    options.deadline = vix::Deadline::earliest(options.deadline, state_->options.deadline);
    bool response_to_head = false;
    {
      std::lock_guard lock(state_->mutex);
      if (state_->closed || !state_->request_received || state_->response_started)
        return vix::Error{invalid_session_state,
                          "HTTPS response preparation requires one received request"};
      response_to_head = state_->request_received && state_->request_method == "HEAD";
    }
    auto bytes = detail::snapshot_response(response, state_->limits, response_to_head);
    if (!bytes)
      return bytes.error();
    try
    {
      auto state = std::make_shared<detail::ResponseState>(
          state_, bytes.take(), response.status_code, std::move(options));
      auto operation = vix::network::detail::OperationFactory<void>::make(
          state_->provider, [state](auto parent)
          { (void)detail::drive_https_response(state, parent); });
      return operation;
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate HTTPS response operation"};
    }
  }

  vix::Result<vix::network::Operation<Response>> https_exchange(
      vix::network::Provider &provider, const vix::network::Endpoint &endpoint,
      const Request &request, const vix::tls::TlsClientConfig &tls,
      ExchangeOptions options)
  {
    const auto default_deadline = vix::Deadline::try_after(
        vix::MonotonicClock::now(), detail::exchange_lifetime);
    if (!default_deadline)
      return vix::Error{vix::network::resource_exhausted,
                        "cannot establish finite HTTPS deadline"};
    options.operation.deadline = vix::Deadline::earliest(
        options.operation.deadline, default_deadline);
    auto request_bytes = detail::snapshot_request(request, options.limits);
    if (!request_bytes)
      return request_bytes.error();
    auto core = vix::network::detail::NetworkAccess::core_for_http(provider);
    if (!core)
      return vix::Error{vix::network::provider_stopped,
                        "Network provider is unavailable"};
    try
    {
      auto connect = provider.connect(endpoint, options.operation);
      auto state = std::make_shared<detail::ClientState>(
          std::move(connect), request_bytes.take(), core, tls, options.operation,
          std::min(options.limits.response_body_bytes, detail::maximum_body));
      auto operation = vix::network::detail::OperationFactory<Response>::make(
          core, [state](auto parent)
          { (void)detail::drive_https_client(state, parent); });
      return operation;
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate HTTPS client operation"};
    }
  }

  vix::Result<vix::network::Operation<HttpsServerSession>> https_accept(
      vix::network::Provider &provider, vix::network::TcpListener &listener,
      const vix::tls::TlsServerConfig &tls, ServerOptions options)
  {
    const auto default_deadline = vix::Deadline::try_after(
        vix::MonotonicClock::now(), detail::exchange_lifetime);
    if (!default_deadline)
      return vix::Error{vix::network::resource_exhausted,
                        "cannot establish finite HTTPS session deadline"};
    options.operation.deadline = vix::Deadline::earliest(
        options.operation.deadline, default_deadline);
    auto core = vix::network::detail::NetworkAccess::core_for_http(provider);
    if (!core)
      return vix::Error{vix::network::provider_stopped,
                        "Network provider is unavailable"};
    try
    {
      auto child = listener.accept(options.operation);
      auto state = std::make_shared<detail::AcceptState>(
          std::move(child), core, tls, options.limits, options.operation);
      auto operation = vix::network::detail::OperationFactory<HttpsServerSession>::make(
          core, [state](auto parent)
          { (void)detail::drive_https_accept(state, parent); });
      return operation;
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate HTTPS accept operation"};
    }
  }
}
