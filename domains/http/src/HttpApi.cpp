#include <vix/http/Http.hpp>

#include "Engine.hpp"
#include "Http1.hpp"
#include "HttpApiInternal.hpp"
#include "PrivateExchange.hpp"

#include <vix/MonotonicClock.hpp>
#include <vix/network/Errors.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <coroutine>
#include <exception>
#include <mutex>
#include <utility>

namespace vix::http::detail
{
  class SessionState
  {
  public:
    SessionState(vix::network::detail::TcpTransportLease transport,
                 std::shared_ptr<vix::network::detail::ProviderCore> owner,
                 vix::http::Limits configured, vix::network::OperationOptions defaults)
        : lease(std::move(transport)), provider(std::move(owner)), limits(configured),
          options(std::move(defaults)) {}

    vix::network::detail::TcpTransportLease lease;
    std::shared_ptr<vix::network::detail::ProviderCore> provider;
    vix::http::Limits limits;
    vix::network::OperationOptions options;
    std::mutex mutex;
    bool request_started{};
    bool request_received{};
    bool response_started{};
    bool closed{};
    std::string request_method;

    vix::Result<vix::network::Operation<std::size_t>> receive(
        std::span<std::byte> buffer, vix::network::OperationOptions opts)
    {
      std::lock_guard lock(mutex);
      if (closed)
        return vix::Error{vix::network::peer_closed, "HTTP session is closed"};
      return lease.receive(buffer, std::move(opts));
    }

    vix::Result<vix::network::Operation<void>> send(
        std::span<const std::byte> buffer, vix::network::OperationOptions opts)
    {
      std::lock_guard lock(mutex);
      if (closed)
        return vix::Error{vix::network::peer_closed, "HTTP session is closed"};
      return lease.send(buffer, std::move(opts));
    }
  };

  struct SessionAccess
  {
    static ServerSession make(std::shared_ptr<SessionState> state)
    { return ServerSession{std::move(state)}; }
  };

  namespace
  {
    constexpr vix::Duration exchange_lifetime = vix::Duration::seconds(30);
    constexpr std::size_t max_body = 65536;
    constexpr std::size_t max_wire_message = 8192;

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
        auto owner = provider;
        if (owner && owner->post([continuation] { continuation.resume(); }))
          return true;
        queued = false;
        return false;
      }
      bool await_resume() const noexcept { return queued; }
    };

    void retire_session(const std::shared_ptr<SessionState> &session) noexcept
    {
      {
        std::lock_guard lock(session->mutex);
        if (session->closed)
          return;
        session->closed = true;
      }
      session->lease.close();
    }

    template <class T>
    void complete(const std::shared_ptr<vix::network::detail::OperationRecord<T>> &record,
                  vix::Completion<T> result)
    {
      record->complete(std::move(result));
    }

    bool has_framing_header(const std::vector<vix::http::HeaderField> &fields)
    {
      for (const auto &field : fields)
      {
        auto lower = field.name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lower == "content-length" || lower == "transfer-encoding")
          return true;
      }
      return false;
    }

    bool has_connection_header(const std::vector<vix::http::HeaderField> &fields)
    {
      for (const auto &field : fields)
      {
        auto lower = field.name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lower == "connection")
          return true;
      }
      return false;
    }

    bool to_private(const std::vector<vix::http::HeaderField> &source,
                    std::vector<HeaderField> &target)
    {
      constexpr std::size_t maximum_fields = 100;
      constexpr std::size_t maximum_bytes = 32768;
      if (source.size() > maximum_fields)
        return false;
      std::size_t total = 0;
      target.reserve(source.size());
      for (const auto &field : source)
      {
        if (field.name.size() > maximum_bytes - total)
          return false;
        total += field.name.size();
        if (field.value.size() > maximum_bytes - total)
          return false;
        total += field.value.size();
        target.push_back({field.name, field.value});
      }
      return true;
    }

    std::vector<vix::http::HeaderField> from_private(const std::vector<HeaderField> &source)
    {
      std::vector<vix::http::HeaderField> result;
      result.reserve(source.size());
      for (const auto &field : source)
        result.push_back({field.name, field.value});
      return result;
    }

    vix::Result<std::vector<std::byte>> serialize_request(const Request &request,
                                                          const vix::http::Limits &limits)
    {
      if (request.version != "HTTP/1.1")
        return vix::Error{unsupported_message, "only HTTP/1.1 request serialization is supported"};
      if (request.body.size() > std::min(limits.request_body_bytes, max_body))
        return vix::Error{message_too_large, "request body exceeds configured limit"};
      if (!request.trailers.empty() || has_framing_header(request.headers) ||
          has_connection_header(request.headers))
        return vix::Error{unsupported_message,
                          "caller framing/connection fields and outgoing trailers are unsupported"};
      std::vector<HeaderField> fields;
      std::string head;
      try
      {
        if (!to_private(request.headers, fields))
          return vix::Error{message_too_large, "request headers exceed configured parser limits"};
        fields.push_back({"Connection", "close"});
        if (!serialize_request_head(request.method, request.target, fields,
                                    {OutgoingFraming::Kind::fixed_length,
                                     request.body.size()}, head))
          return vix::Error{invalid_message, "invalid HTTP request fields or framing"};
        if (request.body.size() + head.size() >
            std::min(limits.serialized_message_bytes, max_wire_message))
          return vix::Error{message_too_large, "serialized HTTP request exceeds limit"};
        std::vector<std::byte> bytes;
        bytes.reserve(head.size() + request.body.size());
        for (const unsigned char c : head)
          bytes.push_back(static_cast<std::byte>(c));
        bytes.insert(bytes.end(), request.body.begin(), request.body.end());
        return bytes;
      }
      catch (...)
      {
        return vix::Error{vix::network::resource_exhausted,
                          "cannot snapshot HTTP request"};
      }
    }

    vix::Result<std::vector<std::byte>> serialize_response(const Response &response,
                                                           const vix::http::Limits &limits,
                                                           bool response_to_head)
    {
      if (response.version != "HTTP/1.1")
        return vix::Error{unsupported_message, "only HTTP/1.1 response serialization is supported"};
      if (response.body.size() > std::min(limits.response_body_bytes, max_body))
        return vix::Error{message_too_large, "response body exceeds configured limit"};
      if (!response.trailers.empty() || has_framing_header(response.headers) ||
          has_connection_header(response.headers))
        return vix::Error{unsupported_message,
                          "caller framing/connection fields and outgoing trailers are unsupported"};
      std::vector<HeaderField> fields;
      std::string head;
      try
      {
        if (!to_private(response.headers, fields))
          return vix::Error{message_too_large, "response headers exceed configured parser limits"};
        fields.push_back({"Connection", "close"});
        const bool status_forbids_body =
            (response.status_code >= 100 && response.status_code < 200) ||
            response.status_code == 204 || response.status_code == 304;
        if (response.status_code < 200)
          return vix::Error{unsupported_message,
                            "informational and protocol-switch responses are not server finals"};
        if (status_forbids_body && !response.body.empty())
          return vix::Error{invalid_message, "status does not permit a response body"};
        if (!serialize_response_head(response.status_code, response.reason, fields,
                                     status_forbids_body
                                         ? OutgoingFraming{OutgoingFraming::Kind::no_body, 0}
                                         : OutgoingFraming{OutgoingFraming::Kind::fixed_length,
                                                           response.body.size()},
                                     head))
          return vix::Error{invalid_message, "invalid HTTP response fields or framing"};
        if (response.body.size() + head.size() >
            std::min(limits.serialized_message_bytes, max_wire_message))
          return vix::Error{message_too_large, "serialized HTTP response exceeds limit"};
        std::vector<std::byte> bytes;
        const bool transmit_body = !status_forbids_body && !response_to_head;
        bytes.reserve(head.size() + (transmit_body ? response.body.size() : 0));
        for (const unsigned char c : head)
          bytes.push_back(static_cast<std::byte>(c));
        if (transmit_body)
          bytes.insert(bytes.end(), response.body.begin(), response.body.end());
        return bytes;
      }
      catch (...)
      {
        return vix::Error{vix::network::resource_exhausted,
                          "cannot snapshot HTTP response"};
      }
    }

    struct ClientState
    {
      ClientState(vix::network::Operation<vix::network::TcpConnection> child,
                  std::vector<std::byte> bytes, std::shared_ptr<vix::network::detail::ProviderCore> owner,
                  vix::network::OperationOptions opts, std::size_t body_limit)
          : connect(std::move(child)), request(std::move(bytes)), provider(std::move(owner)),
            options(std::move(opts)), response_limit(body_limit) {}
      vix::network::Operation<vix::network::TcpConnection> connect;
      std::vector<std::byte> request;
      std::shared_ptr<vix::network::detail::ProviderCore> provider;
      vix::network::OperationOptions options;
      std::size_t response_limit{};
    };

    Detached drive_client(std::shared_ptr<ClientState> state,
                          std::shared_ptr<vix::network::detail::OperationRecord<Response>> parent)
    {
      try
      {
        auto connected = co_await std::move(state->connect);
        if (connected.status() != vix::CompletionStatus::Success)
        {
          if (connected.status() == vix::CompletionStatus::Cancelled)
            complete(parent, vix::Completion<Response>::cancelled());
          else
            complete(parent, vix::Completion<Response>{connected.error()});
          co_return;
        }
        auto connection = connected.take();
        if (!(co_await ProviderHandoff{state->provider}))
        {
          complete(parent, vix::Completion<Response>{vix::Error{
              vix::network::provider_stopped, "Network provider stopped before HTTP exchange"}});
          co_return;
        }
        auto child = start_private_exchange(
            std::move(connection), state->request, state->options,
            vix::Duration::zero(), default_exchange_receive_chunk, false,
            state->response_limit);
        if (!child)
        {
          complete(parent, vix::Completion<Response>{child.error()});
          co_return;
        }
        auto terminal = co_await child.take();
        if (terminal.status() != vix::CompletionStatus::Success)
        {
          if (terminal.status() == vix::CompletionStatus::Cancelled)
            complete(parent, vix::Completion<Response>::cancelled());
          else
            complete(parent, vix::Completion<Response>{terminal.error()});
          co_return;
        }
        auto result = terminal.take();
        Response response;
        response.version = std::move(result.version);
        response.status_code = result.status_code;
        response.reason = std::move(result.reason);
        response.headers = from_private(result.headers);
        response.body = std::move(result.body);
        response.trailers = from_private(result.trailers);
        complete(parent, vix::Completion<Response>{std::move(response)});
      }
      catch (...)
      {
        complete(parent, vix::Completion<Response>{vix::Error{
            vix::network::resource_exhausted,
            "exception while progressing HTTP client exchange"}});
      }
    }

    struct RequestState
    {
      RequestState(std::shared_ptr<SessionState> owner, vix::network::OperationOptions opts)
          : session(std::move(owner)), options(std::move(opts)) {}
      std::shared_ptr<SessionState> session;
      vix::network::OperationOptions options;
      Http1Parser parser;
      std::array<std::byte, 4096> input{};
      std::array<std::byte, 4096> body_chunk{};
      std::vector<std::byte> body;
    };

    Detached drive_request(std::shared_ptr<RequestState> state,
                           std::shared_ptr<vix::network::detail::OperationRecord<Request>> parent)
    {
      auto session = state->session;
      {
        std::lock_guard lock(session->mutex);
        if (session->closed || session->request_started)
        {
          complete(parent, vix::Completion<Request>{vix::Error{
              invalid_session_state, "HTTP session cannot receive another request"}});
          co_return;
        }
        session->request_started = true;
      }
      try
      {
        for (;;)
        {
          auto step = state->parser.next(
              MessageKind::request, {},
              std::span<std::byte>{state->body_chunk}.first(
                  std::min(state->body_chunk.size(),
                           std::min(session->limits.request_body_bytes, max_body) -
                               std::min(state->body.size(),
                                        std::min(session->limits.request_body_bytes, max_body)))),
              false);
          if (step.status == ParseStatus::headers_complete)
          {
            if (state->parser.head().body_mode == BodyMode::fixed_length &&
                state->parser.head().content_length >
                    std::min(session->limits.request_body_bytes, max_body))
            {
              retire_session(session);
              complete(parent, vix::Completion<Request>{vix::Error{
                  message_too_large, "HTTP request body exceeds configured limit"}});
              co_return;
            }
            continue;
          }
          if (step.status == ParseStatus::body_data)
          {
            if (step.body_bytes == 0)
            {
              retire_session(session);
              complete(parent, vix::Completion<Request>{vix::Error{
                  message_too_large, "HTTP request body exceeds configured limit"}});
              co_return;
            }
            state->body.insert(state->body.end(), state->body_chunk.begin(),
                               state->body_chunk.begin() +
                                   static_cast<std::ptrdiff_t>(step.body_bytes));
            continue;
          }
          if (step.status == ParseStatus::protocol_error)
          {
            retire_session(session);
            complete(parent, vix::Completion<Request>{vix::Error{
                invalid_message, "malformed or ambiguous HTTP request"}});
            co_return;
          }
          if (step.status == ParseStatus::message_complete)
          {
            if (state->parser.buffered_bytes() != 0)
            {
              retire_session(session);
              complete(parent, vix::Completion<Request>{vix::Error{
                  invalid_message, "surplus bytes follow one-request session message"}});
              co_return;
            }
            Request request;
            request.method = state->parser.head().method;
            request.target = state->parser.head().target;
            request.version = state->parser.head().version;
            request.headers = from_private(state->parser.head().fields);
            request.trailers = from_private(state->parser.head().trailers);
            request.body = std::move(state->body);
            {
              std::lock_guard lock(session->mutex);
              session->request_received = true;
              session->request_method = request.method;
            }
            complete(parent, vix::Completion<Request>{std::move(request)});
            co_return;
          }
          if (!(co_await ProviderHandoff{session->provider}))
          {
            retire_session(session);
            complete(parent, vix::Completion<Request>{vix::Error{
                vix::network::provider_stopped, "Network provider stopped during HTTP request"}});
            co_return;
          }
          auto receive_operation = session->receive(state->input, state->options);
          if (!receive_operation)
          {
            retire_session(session);
            complete(parent, vix::Completion<Request>{receive_operation.error()});
            co_return;
          }
          auto received = co_await receive_operation.take();
          if (received.status() != vix::CompletionStatus::Success)
          {
            retire_session(session);
            if (received.status() == vix::CompletionStatus::Cancelled)
              complete(parent, vix::Completion<Request>::cancelled());
            else
              complete(parent, vix::Completion<Request>{received.error()});
            co_return;
          }
          const auto count = received.take();
          if (count == 0 || state->parser.append(
                  std::span<const std::byte>{state->input.data(), count}) != count)
          {
            retire_session(session);
            complete(parent, vix::Completion<Request>{vix::Error{
                invalid_message, "incomplete or oversized HTTP request"}});
            co_return;
          }
        }
      }
      catch (...)
      {
        retire_session(session);
        complete(parent, vix::Completion<Request>{vix::Error{
            vix::network::resource_exhausted, "exception while parsing HTTP request"}});
      }
    }

    struct ResponseState
    {
      ResponseState(std::shared_ptr<SessionState> owner,
                    std::vector<std::byte> data,
                    unsigned response_status,
                    vix::network::OperationOptions opts)
          : session(std::move(owner)), bytes(std::move(data)), status(response_status),
            options(std::move(opts)) {}
      std::shared_ptr<SessionState> session;
      std::vector<std::byte> bytes;
      unsigned status{};
      vix::network::OperationOptions options;
    };

    Detached drive_response(std::shared_ptr<ResponseState> state,
                            std::shared_ptr<vix::network::detail::OperationRecord<void>> parent)
    {
      auto session = state->session;
      {
        std::lock_guard lock(session->mutex);
        if (session->closed || !session->request_received || session->response_started)
        {
          complete(parent, vix::Completion<void>{vix::Error{
              invalid_session_state, "HTTP response requires one received request"}});
          co_return;
        }
        if (session->request_method == "CONNECT" &&
            state->status >= 200 && state->status < 300)
        {
          complete(parent, vix::Completion<void>{vix::Error{
              unsupported_message, "CONNECT tunneling is not supported"}});
          co_return;
        }
        session->response_started = true;
      }
      if (!(co_await ProviderHandoff{session->provider}))
      {
        retire_session(session);
        complete(parent, vix::Completion<void>{vix::Error{
            vix::network::provider_stopped, "Network provider stopped before HTTP response"}});
        co_return;
      }
      auto send_operation = session->send(state->bytes, state->options);
      if (!send_operation)
      {
        retire_session(session);
        complete(parent, vix::Completion<void>{send_operation.error()});
        co_return;
      }
      auto sent = co_await send_operation.take();
      retire_session(session);
      {
        std::lock_guard lock(session->mutex);
        session->closed = true;
      }
      if (sent.status() == vix::CompletionStatus::Success)
        complete(parent, vix::Completion<void>::success());
      else if (sent.status() == vix::CompletionStatus::Cancelled)
        complete(parent, vix::Completion<void>::cancelled());
      else
        complete(parent, vix::Completion<void>{sent.error()});
    }

    struct AcceptState
    {
      AcceptState(vix::network::Operation<vix::network::TcpConnection> child,
                  std::shared_ptr<vix::network::detail::ProviderCore> owner,
      vix::http::Limits configured, vix::network::OperationOptions opts)
          : accept_operation(std::move(child)), provider(std::move(owner)),
            limits(configured), options(std::move(opts)) {}
      vix::network::Operation<vix::network::TcpConnection> accept_operation;
      std::shared_ptr<vix::network::detail::ProviderCore> provider;
      vix::http::Limits limits;
      vix::network::OperationOptions options;
    };

    Detached drive_accept(std::shared_ptr<AcceptState> state,
                          std::shared_ptr<vix::network::detail::OperationRecord<ServerSession>> parent)
    {
      try
      {
        auto accepted = co_await std::move(state->accept_operation);
        if (accepted.status() != vix::CompletionStatus::Success)
        {
          if (accepted.status() == vix::CompletionStatus::Cancelled)
            complete(parent, vix::Completion<ServerSession>::cancelled());
          else
            complete(parent, vix::Completion<ServerSession>{accepted.error()});
          co_return;
        }
        auto connection = accepted.take();
        auto lease_result = vix::network::detail::NetworkAccess::claim_tcp_for_http(
            std::move(connection));
        if (!lease_result)
        {
          complete(parent, vix::Completion<ServerSession>{lease_result.error()});
          co_return;
        }
        auto lease = lease_result.take();
        auto provider = vix::network::detail::NetworkAccess::provider_for_http_lease(lease);
        auto session = std::make_shared<SessionState>(std::move(lease), provider,
                                                       state->limits, state->options);
        complete(parent, vix::Completion<ServerSession>{
            SessionAccess::make(std::move(session))});
      }
      catch (...)
      {
        complete(parent, vix::Completion<ServerSession>{vix::Error{
            vix::network::resource_exhausted, "exception while accepting HTTP session"}});
      }
    }
  }
  vix::Result<std::vector<std::byte>> snapshot_request(
      const vix::http::Request &request, const vix::http::Limits &limits)
  {
    return serialize_request(request, limits);
  }

  vix::Result<std::vector<std::byte>> snapshot_response(
      const vix::http::Response &response, const vix::http::Limits &limits,
      bool response_to_head)
  {
    return serialize_response(response, limits, response_to_head);
  }

  std::vector<vix::http::HeaderField> public_headers(
      const std::vector<vix::http::detail::HeaderField> &source)
  {
    return from_private(source);
  }
} // namespace vix::http::detail

namespace vix::http
{

  ServerSession::ServerSession(std::shared_ptr<detail::SessionState> state) noexcept
      : state_(std::move(state)) {}
  ServerSession::ServerSession(ServerSession &&) noexcept = default;
  ServerSession &ServerSession::operator=(ServerSession &&) noexcept = default;
  ServerSession::~ServerSession() { close(); }

  void ServerSession::close() noexcept
  {
    if (!state_)
      return;
    detail::retire_session(state_);
  }

  vix::Result<vix::network::Operation<Request>>
  ServerSession::receive_request(vix::network::OperationOptions options)
  {
    if (!state_)
      return vix::Error{invalid_session_state, "HTTP session is empty"};
    options.deadline = vix::Deadline::earliest(options.deadline, state_->options.deadline);
    try
    {
      auto request_state = std::make_shared<detail::RequestState>(state_, options);
      auto operation = vix::network::detail::OperationFactory<Request>::make(
          state_->provider, [request_state](auto parent)
          { (void)detail::drive_request(request_state, parent); });
      return operation;
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate HTTP request operation"};
    }
  }

  vix::Result<vix::network::Operation<void>>
  ServerSession::send_response(const Response &response,
                               vix::network::OperationOptions options)
  {
    if (!state_)
      return vix::Error{invalid_session_state, "HTTP session is empty"};
    options.deadline = vix::Deadline::earliest(options.deadline, state_->options.deadline);
    bool response_to_head = false;
    {
      std::lock_guard lock(state_->mutex);
      if (state_->closed || !state_->request_received || state_->response_started)
        return vix::Error{invalid_session_state,
                          "HTTP response preparation requires one received request"};
      response_to_head = state_->request_received && state_->request_method == "HEAD";
    }
    auto bytes = detail::snapshot_response(response, state_->limits, response_to_head);
    if (!bytes)
      return bytes.error();
    try
    {
      auto response_state = std::make_shared<detail::ResponseState>(
          state_, bytes.take(), response.status_code, std::move(options));
      auto operation = vix::network::detail::OperationFactory<void>::make(
          state_->provider, [response_state](auto parent)
          { (void)detail::drive_response(response_state, parent); });
      return operation;
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate HTTP response operation"};
    }
  }

  vix::Result<vix::network::Operation<Response>> exchange(
      vix::network::Provider &provider, const vix::network::Endpoint &endpoint,
      const Request &request, ExchangeOptions options)
  {
    options.operation.deadline = vix::Deadline::earliest(
        options.operation.deadline,
        vix::Deadline::try_after(vix::MonotonicClock::now(), detail::exchange_lifetime));
    auto request_bytes = detail::snapshot_request(request, options.limits);
    if (!request_bytes)
      return request_bytes.error();
    auto core = vix::network::detail::NetworkAccess::core_for_http(provider);
    if (!core)
      return vix::Error{vix::network::provider_stopped, "Network provider is unavailable"};
    try
    {
      auto connect = provider.connect(endpoint, options.operation);
      auto request_snapshot = request_bytes.take();
      auto client_state = std::make_shared<detail::ClientState>(
          std::move(connect), std::move(request_snapshot), core, options.operation,
          std::min(options.limits.response_body_bytes, detail::max_body));
      auto operation = vix::network::detail::OperationFactory<Response>::make(
          core, [client_state](auto parent)
          { (void)detail::drive_client(client_state, parent); });
      return operation;
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate HTTP client operation"};
    }
  }

  vix::Result<vix::network::Operation<ServerSession>> accept(
      vix::network::Provider &provider, vix::network::TcpListener &listener,
      ServerOptions options)
  {
    options.operation.deadline = vix::Deadline::earliest(
        options.operation.deadline,
        vix::Deadline::try_after(vix::MonotonicClock::now(), detail::exchange_lifetime));
    auto core = vix::network::detail::NetworkAccess::core_for_http(provider);
    if (!core)
      return vix::Error{vix::network::provider_stopped, "Network provider is unavailable"};
    try
    {
      auto child = listener.accept(options.operation);
      auto state = std::make_shared<detail::AcceptState>(
          std::move(child), core, options.limits, options.operation);
      auto operation = vix::network::detail::OperationFactory<ServerSession>::make(
          core, [state](auto parent) { (void)detail::drive_accept(state, parent); });
      return operation;
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate HTTP accept operation"};
    }
  }
} // namespace vix::http
