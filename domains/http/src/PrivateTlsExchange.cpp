#include "PrivateTlsExchange.hpp"

#include "Engine.hpp"
#include "HttpTlsAccess.hpp"

#include <vix/network/Errors.hpp>
#include <vix/tls/Errors.hpp>
#include <vix/MonotonicClock.hpp>

#include <algorithm>
#include <array>
#include <coroutine>
#include <exception>
#include <memory>
#include <utility>

namespace vix::http::detail
{
  namespace
  {
    constexpr std::size_t request_limit = 8192;
    constexpr std::size_t default_response_body_limit = 65536;

    struct ParentFrame
    {
      struct promise_type
      {
        ParentFrame get_return_object() const noexcept { return {}; }
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

    struct HttpsState
    {
      HttpsState(vix::tls::TlsChannel owned_channel,
                 std::shared_ptr<vix::network::detail::ProviderCore> owned_provider,
                 std::span<const std::byte> request_bytes,
                 std::string request_method,
                 std::size_t response_limit,
                 std::size_t receive_chunk_for_test,
                 PrivateExchangeTestObserver observer)
          : channel(std::move(owned_channel)), provider(std::move(owned_provider)),
            request(request_bytes.begin(), request_bytes.end()),
            method(std::move(request_method)),
            response_body_limit(response_limit),
            receive_chunk_size(receive_chunk_for_test),
            test_observer(observer),
            parser([]
                   {
                     Limits limits;
                     limits.staged_bytes = 65536;
                     return limits;
                   }())
      {
      }

      vix::tls::TlsChannel channel;
      std::shared_ptr<vix::network::detail::ProviderCore> provider;
      std::vector<std::byte> request;
      std::string method;
      std::size_t response_body_limit{};
      std::size_t receive_chunk_size{};
      PrivateExchangeTestObserver test_observer;
      Http1Parser parser;
      std::array<std::byte, max_exchange_receive_chunk> input{};
      std::array<std::byte, default_response_body_limit> body{};
      std::size_t body_size{};
      InformationalResponseCounter informational_responses;
      bool input_eof{};
    };

    void finish(const std::shared_ptr<HttpsState> &state,
                const std::shared_ptr<
                    vix::network::detail::OperationRecord<ExchangeResponse>> &parent,
                vix::Completion<ExchangeResponse> completion) noexcept
    {
      if (state->test_observer.phase)
        state->test_observer.phase(state->test_observer.context,
                                   "close TLS channel before parent completion");
      state->channel.close();
      if (state->test_observer.phase)
        state->test_observer.phase(state->test_observer.context,
                                   "publish parent completion");
      parent->complete(std::move(completion));
    }

    void mark_phase(const std::shared_ptr<HttpsState> &state,
                    const char *phase) noexcept
    {
      if (state->test_observer.phase)
        state->test_observer.phase(state->test_observer.context, phase);
    }

    vix::Completion<ExchangeResponse> protocol_error(const char *message)
    {
      return vix::Completion<ExchangeResponse>{
          vix::Error{malformed_response, message}};
    }

    ParentFrame drive_exchange(
        std::shared_ptr<HttpsState> state,
        std::shared_ptr<vix::network::detail::OperationRecord<ExchangeResponse>> parent,
        vix::network::OperationOptions options)
    {
      try
      {
        mark_phase(state, "check cancellation/deadline");
        if (options.cancellation.is_cancel_requested())
        {
          finish(state, parent, vix::Completion<ExchangeResponse>::cancelled());
          co_return;
        }
        if (options.deadline && options.deadline->reached())
        {
          finish(state, parent, vix::Completion<ExchangeResponse>{
                                    vix::Error{vix::network::timed_out,
                                               "HTTPS deadline reached before handshake"}});
          co_return;
        }

        mark_phase(state, "await TLS handshake");
        auto handshake = co_await state->channel.handshake(options);
        if (handshake.status() != vix::CompletionStatus::Success)
        {
          finish(state, parent,
                 handshake.status() == vix::CompletionStatus::Cancelled
                     ? vix::Completion<ExchangeResponse>::cancelled()
                     : vix::Completion<ExchangeResponse>{handshake.error()});
          co_return;
        }

        mark_phase(state, "await TLS request send");
        auto sent = co_await state->channel.send(state->request, options);
        if (sent.status() != vix::CompletionStatus::Success)
        {
          finish(state, parent,
                 sent.status() == vix::CompletionStatus::Cancelled
                     ? vix::Completion<ExchangeResponse>::cancelled()
                     : vix::Completion<ExchangeResponse>{sent.error()});
          co_return;
        }

        for (;;)
        {
          mark_phase(state, "parse HTTP response");
          auto step = state->parser.next(
              MessageKind::response, state->method,
              std::span<std::byte>{state->body}.first(state->response_body_limit)
                  .subspan(state->body_size),
              state->input_eof);
          if (step.status == ParseStatus::body_data)
          {
            state->body_size += step.body_bytes;
            continue;
          }
          if (step.status == ParseStatus::message_complete)
          {
            const auto status = state->parser.head().status_code;
            if (status == 101 ||
                (state->method == "CONNECT" && status >= 200 && status < 300))
            {
              finish(state, parent,
                     protocol_error("HTTP protocol transition is unsupported by private HTTPS exchange"));
              co_return;
            }
            if (status >= 100 && status < 200)
            {
              if (!state->informational_responses.consume())
              {
                finish(state, parent,
                       vix::Completion<ExchangeResponse>{
                           vix::Error{informational_response_limit_exceeded,
                                      "HTTPS informational response limit exceeded"}});
                co_return;
              }
              if (!state->parser.reset_message())
              {
                finish(state, parent,
                       protocol_error("cannot advance past informational HTTPS response"));
                co_return;
              }
              state->body_size = 0;
              continue;
            }
            if (state->parser.buffered_bytes() != 0)
            {
              finish(state, parent,
                     protocol_error("surplus bytes follow the single HTTPS response"));
              co_return;
            }
            ExchangeResponse response;
            response.version = state->parser.head().version;
            response.status_code = state->parser.head().status_code;
            response.reason = state->parser.head().reason;
            response.headers = state->parser.head().fields;
            response.trailers = state->parser.head().trailers;
            response.body.assign(state->body.begin(),
                                 state->body.begin() +
                                     static_cast<std::ptrdiff_t>(state->body_size));
            // The exchange owns the secure channel. Complete a mutual
            // close_notify exchange before publishing a successful parent.
            mark_phase(state, "await TLS close_notify exchange");
            auto shutdown = co_await state->channel.shutdown(options);
            if (shutdown.status() != vix::CompletionStatus::Success)
            {
              finish(state, parent,
                     shutdown.status() == vix::CompletionStatus::Cancelled
                         ? vix::Completion<ExchangeResponse>::cancelled()
                         : vix::Completion<ExchangeResponse>{shutdown.error()});
              co_return;
            }
            finish(state, parent,
                   vix::Completion<ExchangeResponse>{std::move(response)});
            co_return;
          }
          if (step.status == ParseStatus::protocol_error)
          {
            finish(state, parent,
                   protocol_error("malformed or ambiguous HTTPS response"));
            co_return;
          }
          if (step.status == ParseStatus::headers_complete)
          {
            if (state->parser.head().body_mode == BodyMode::tunnel)
            {
              finish(state, parent,
                     protocol_error("HTTP tunnel transition is unsupported by private HTTPS exchange"));
              co_return;
            }
            if (state->parser.head().body_mode == BodyMode::close_delimited)
            {
              finish(state, parent, vix::Completion<ExchangeResponse>{
                                        vix::Error{unsupported_response_framing,
                                                   "private HTTPS exchange requires explicit response framing"}});
              co_return;
            }
            if (state->parser.head().body_mode == BodyMode::fixed_length &&
                state->parser.head().content_length > state->response_body_limit)
            {
              finish(state, parent, vix::Completion<ExchangeResponse>{
                                        vix::Error{response_too_large,
                                                   "HTTPS response body exceeds private limit"}});
              co_return;
            }
            continue;
          }
          if (state->body_size == state->response_body_limit)
          {
            finish(state, parent, vix::Completion<ExchangeResponse>{
                                      vix::Error{response_too_large,
                                                 "HTTPS response body exceeds private limit"}});
            co_return;
          }

          if (!(co_await ProviderHandoff{state->provider}))
          {
            finish(state, parent, vix::Completion<ExchangeResponse>{
                                      vix::Error{vix::network::provider_stopped,
                                                 "cannot resume HTTPS exchange on Network provider"}});
            co_return;
          }
          if (state->input_eof)
          {
            finish(state, parent,
                   protocol_error("TLS closed before HTTP final response completed"));
            co_return;
          }
          mark_phase(state, "await TLS response receive");
          auto received = co_await state->channel.receive(
              std::span<std::byte>{state->input}.first(state->receive_chunk_size),
              options);
          if (received.status() == vix::CompletionStatus::Failure &&
              received.error().identity() == vix::tls::peer_closed)
          {
            state->input_eof = true;
            continue;
          }
          if (received.status() != vix::CompletionStatus::Success)
          {
            finish(state, parent,
                   received.status() == vix::CompletionStatus::Cancelled
                       ? vix::Completion<ExchangeResponse>::cancelled()
                       : vix::Completion<ExchangeResponse>{received.error()});
            co_return;
          }
          const auto count = received.take();
          if (count == 0 || state->parser.append(
                                 std::span<const std::byte>{state->input.data(), count}) != count)
          {
            finish(state, parent,
                   protocol_error("HTTPS response exceeds staged input limit"));
            co_return;
          }
        }
      }
      catch (...)
      {
        finish(state, parent,
               vix::Completion<ExchangeResponse>{
                   vix::Error{vix::network::io_failure,
                              "exception escaped private HTTPS exchange"}});
      }
    }
  } // namespace

  vix::Result<vix::network::Operation<ExchangeResponse>> start_private_tls_exchange(
      vix::tls::TlsChannel &&channel,
      std::span<const std::byte> serialized_request,
      vix::network::OperationOptions options,
      vix::Duration exchange_lifetime,
      std::size_t receive_chunk_for_test,
      PrivateExchangeTestObserver test_observer,
      bool establish_default_deadline,
      std::size_t response_body_limit)
  {
    if (receive_chunk_for_test == 0 ||
        receive_chunk_for_test > max_exchange_receive_chunk)
      return vix::Error{vix::network::resource_exhausted,
                        "HTTPS exchange receive chunk exceeds private bound"};
    if (establish_default_deadline)
    {
      const auto default_deadline =
          vix::Deadline::try_after(vix::MonotonicClock::now(), exchange_lifetime);
      if (!default_deadline)
        return vix::Error{vix::network::resource_exhausted,
                          "cannot establish finite HTTPS exchange deadline"};
      options.deadline = vix::Deadline::earliest(options.deadline,
                                                 default_deadline);
    }

    if (serialized_request.empty() || serialized_request.size() > request_limit)
      return vix::Error{response_too_large,
                        "serialized HTTP request is empty or exceeds private limit"};
    if (response_body_limit > default_response_body_limit)
      return vix::Error{response_too_large,
                        "HTTPS response body limit is outside the private bound"};
    std::string request_method;
    try
    {
      if (!validated_request_method(serialized_request, request_method))
        return vix::Error{malformed_request,
                          "private HTTPS exchange requires one valid complete request"};
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot validate private HTTPS request"};
    }
    auto provider = vix::tls::detail::HttpTlsAccess::provider(channel);
    if (!provider)
      return vix::Error{vix::tls::channel_closed,
                        "TLS channel has no active Network provider"};
    try
    {
      auto state = std::make_shared<HttpsState>(
          std::move(channel), provider, serialized_request, std::move(request_method),
          response_body_limit, receive_chunk_for_test, test_observer);
      auto operation = vix::network::detail::OperationFactory<ExchangeResponse>::make(
          provider, [state, options = std::move(options)](auto parent) mutable
          {
            if (state->provider->cooperative() &&
                !state->provider->on_owner_thread())
            {
              finish(state, parent, vix::Completion<ExchangeResponse>{
                                        vix::Error{vix::network::operation_in_progress,
                                                   "cooperative HTTPS exchange is thread-affine"}});
              return;
            }
            (void)drive_exchange(state, parent, std::move(options));
          });
      return vix::Result<vix::network::Operation<ExchangeResponse>>{
          std::move(operation)};
    }
    catch (...)
    {
      channel.close();
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate private HTTPS exchange state"};
    }
  }
} // namespace vix::http::detail
