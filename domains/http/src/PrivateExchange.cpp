#include "PrivateExchange.hpp"

#include "Engine.hpp"

#include <vix/network/Errors.hpp>
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
    constexpr std::size_t response_body_limit = 65536;

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

    // Network's TCP starter arms a descriptor synchronously.  After one child
    // completes, return progression to the provider command queue before
    // starting the next child: a queued stale readiness callback for the
    // previous direction must not contend with a starter holding the
    // connection mutex while it waits for descriptor registration.
    struct ProviderHandoff
    {
      std::shared_ptr<vix::network::detail::ProviderCore> provider;
      bool queued{true};

      bool await_ready() const noexcept { return false; }
      bool await_suspend(std::coroutine_handle<> continuation)
      {
        // Once the handle is published, the callback may resume and destroy
        // this awaiter before post() returns.  Keep success entirely local;
        // only write the failure flag when no callback was accepted.
        auto owner = provider;
        if (owner && owner->post([continuation] { continuation.resume(); }))
          return true;
        queued = false;
        return false;
      }
      bool await_resume() const noexcept { return queued; }
    };

    struct ExchangeState
    {
      ExchangeState(vix::network::detail::TcpTransportLease owned_lease,
                    std::shared_ptr<vix::network::detail::ProviderCore> owned_provider,
                    std::span<const std::byte> request_bytes,
                    std::string request_method,
                    std::size_t receive_chunk_for_test,
                    std::size_t response_body_limit_for_exchange)
          : lease(std::move(owned_lease)), provider(std::move(owned_provider)),
            request(request_bytes.begin(), request_bytes.end()),
            method(std::move(request_method)),
            receive_chunk_size(receive_chunk_for_test),
            response_limit(response_body_limit_for_exchange)
      {
      }

      vix::network::detail::TcpTransportLease lease;
      std::shared_ptr<vix::network::detail::ProviderCore> provider;
      std::vector<std::byte> request;
      std::string method;
      std::size_t receive_chunk_size{};
      std::size_t response_limit{};
      Http1Parser parser{[]
                         {
                           Limits limits;
                           limits.staged_bytes = 65536;
                           return limits;
                         }()};
      std::array<std::byte, max_exchange_receive_chunk> input{};
      std::array<std::byte, response_body_limit> body{};
      std::size_t body_size{};
      InformationalResponseCounter informational_responses;
      bool input_eof{};
    };

    void finish(const std::shared_ptr<ExchangeState> &state,
                const std::shared_ptr<vix::network::detail::OperationRecord<ExchangeResponse>> &parent,
                vix::Completion<ExchangeResponse> completion) noexcept
    {
      if (state->provider->cooperative() && !state->provider->on_owner_thread())
      {
        if (!state->provider->post([state] { state->lease.close(); }))
          std::terminate(); // A live lease prevents normal provider shutdown.
      }
      else
        state->lease.close();
      parent->complete(std::move(completion));
    }

    vix::Completion<ExchangeResponse> protocol_error(const char *message)
    {
      return vix::Completion<ExchangeResponse>{
          vix::Error{malformed_response, message}};
    }

    ParentFrame drive_exchange(
        std::shared_ptr<ExchangeState> state,
        std::shared_ptr<vix::network::detail::OperationRecord<ExchangeResponse>> parent,
        vix::network::OperationOptions options)
    {
      try
      {
        if (options.cancellation.is_cancel_requested())
        {
          finish(state, parent, vix::Completion<ExchangeResponse>::cancelled());
          co_return;
        }
        if (options.deadline && options.deadline->reached())
        {
          finish(state, parent,
                 vix::Completion<ExchangeResponse>{
                     vix::Error{vix::network::timed_out,
                                "HTTP exchange deadline reached before send"}});
          co_return;
        }

        auto sent = co_await state->lease.send(state->request, options);
        if (sent.status() != vix::CompletionStatus::Success)
        {
          if (sent.status() == vix::CompletionStatus::Cancelled)
            finish(state, parent, vix::Completion<ExchangeResponse>::cancelled());
          else
            finish(state, parent, vix::Completion<ExchangeResponse>{sent.error()});
          co_return;
        }

        for (;;)
        {
          auto step = state->parser.next(
              MessageKind::response, state->method,
              std::span<std::byte>{state->body}.subspan(
                  std::min(state->body_size, state->response_limit),
                  state->response_limit - std::min(state->body_size, state->response_limit)),
              state->input_eof);
          if (step.status == ParseStatus::body_data)
          {
            if (step.body_bytes == 0)
            {
              finish(state, parent,
                     vix::Completion<ExchangeResponse>{
                         vix::Error{response_too_large,
                                    "HTTP response body exceeds configured limit"}});
              co_return;
            }
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
                     protocol_error("HTTP protocol transition is unsupported by private exchange"));
              co_return;
            }
            if (status >= 100 && status < 200)
            {
              if (!state->informational_responses.consume())
              {
                finish(state, parent,
                       vix::Completion<ExchangeResponse>{
                           vix::Error{informational_response_limit_exceeded,
                                      "HTTP informational response limit exceeded"}});
                co_return;
              }
              if (!state->parser.reset_message())
              {
                finish(state, parent,
                       protocol_error("cannot advance past informational response"));
                co_return;
              }
              state->body_size = 0;
              continue;
            }
            if (state->parser.buffered_bytes() != 0)
            {
              finish(state, parent,
              protocol_error("surplus bytes follow the single HTTP response"));
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
            finish(state, parent,
                   vix::Completion<ExchangeResponse>{std::move(response)});
            co_return;
          }
          if (step.status == ParseStatus::protocol_error)
          {
            finish(state, parent,
                   protocol_error("malformed or ambiguous HTTP response"));
            co_return;
          }
          if (step.status == ParseStatus::headers_complete)
          {
            if (state->parser.head().body_mode == BodyMode::fixed_length &&
                state->parser.head().content_length > state->response_limit)
            {
              finish(state, parent,
                     vix::Completion<ExchangeResponse>{
                         vix::Error{response_too_large,
                                    "HTTP response body exceeds private test limit"}});
              co_return;
            }
            continue;
          }
          if (state->body_size == state->response_limit)
          {
            finish(state, parent,
                   vix::Completion<ExchangeResponse>{
                       vix::Error{response_too_large,
                                  "HTTP response body exceeds private test limit"}});
            co_return;
          }

          if (!(co_await ProviderHandoff{state->provider}))
          {
            finish(state, parent,
                   vix::Completion<ExchangeResponse>{
                       vix::Error{vix::network::provider_stopped,
                                  "cannot resume HTTP exchange on its Network provider"}});
            co_return;
          }
          if (state->input_eof)
          {
            finish(state, parent,
                   protocol_error("peer closed before HTTP final response completed"));
            co_return;
          }
          auto received = co_await state->lease.receive(
              std::span<std::byte>{state->input}.first(state->receive_chunk_size),
              options);
          if (received.status() != vix::CompletionStatus::Success)
          {
            if (received.status() == vix::CompletionStatus::Cancelled)
              finish(state, parent, vix::Completion<ExchangeResponse>::cancelled());
            else
              finish(state, parent,
                     vix::Completion<ExchangeResponse>{received.error()});
            co_return;
          }
          const auto count = received.take();
          if (count == 0)
          {
            state->input_eof = true;
            continue;
          }
          if (state->parser.append(
                  std::span<const std::byte>{state->input.data(), count}) != count)
          {
            finish(state, parent,
                   protocol_error("HTTP response exceeds staged input limit"));
            co_return;
          }
        }
      }
      catch (...)
      {
        finish(state, parent,
               vix::Completion<ExchangeResponse>{
                   vix::Error{vix::network::io_failure,
                              "exception escaped private HTTP exchange"}});
      }
    }
  } // namespace

  vix::Result<vix::network::Operation<ExchangeResponse>> start_private_exchange(
      vix::network::TcpConnection &&connection,
      std::span<const std::byte> serialized_request,
      vix::network::OperationOptions options,
      vix::Duration exchange_lifetime,
      std::size_t receive_chunk_for_test,
      bool establish_default_deadline,
      std::size_t response_body_limit_for_exchange)
  {
    if (receive_chunk_for_test == 0 ||
        receive_chunk_for_test > max_exchange_receive_chunk)
      return vix::Error{vix::network::resource_exhausted,
                        "HTTP exchange receive chunk exceeds private bound"};
    if (response_body_limit_for_exchange > response_body_limit)
      response_body_limit_for_exchange = response_body_limit;
    if (establish_default_deadline)
    {
      const auto default_deadline =
          vix::Deadline::try_after(vix::MonotonicClock::now(), exchange_lifetime);
      if (!default_deadline)
        return vix::Error{vix::network::resource_exhausted,
                          "cannot establish finite HTTP exchange deadline"};
      options.deadline = vix::Deadline::earliest(options.deadline,
                                                 default_deadline);
    }

    if (serialized_request.empty() || serialized_request.size() > request_limit)
      return vix::Error{response_too_large,
                        "serialized HTTP request is empty or exceeds private limit"};

    std::string request_method;
    try
    {
      if (!validated_request_method(serialized_request, request_method))
        return vix::Error{malformed_request,
                          "private HTTP exchange requires one valid complete request"};
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot validate private HTTP request"};
    }

    auto claimed = vix::network::detail::NetworkAccess::claim_tcp_for_http(
        std::move(connection));
    if (!claimed)
      return claimed.error();

    auto lease = claimed.take();
    auto provider =
        vix::network::detail::NetworkAccess::provider_for_http_lease(lease);
    if (!provider)
      return vix::Error{vix::network::provider_stopped,
                        "HTTP transport has no active provider"};

    try
    {
      auto state = std::make_shared<ExchangeState>(
          std::move(lease), provider, serialized_request, std::move(request_method),
          receive_chunk_for_test, response_body_limit_for_exchange);
      auto operation = vix::network::detail::OperationFactory<ExchangeResponse>::make(
          provider, [state, options = std::move(options)](auto parent) mutable
          {
            if (state->provider->cooperative() &&
                !state->provider->on_owner_thread())
            {
              finish(state, parent,
                     vix::Completion<ExchangeResponse>{
                         vix::Error{vix::network::operation_in_progress,
                                    "cooperative HTTP exchange is thread-affine"}});
              return;
            }
            (void)drive_exchange(state, parent, std::move(options));
          });
      return vix::Result<vix::network::Operation<ExchangeResponse>>{std::move(operation)};
    }
    catch (...)
    {
      return vix::Error{vix::network::resource_exhausted,
                        "cannot allocate private HTTP exchange state"};
    }
  }
} // namespace vix::http::detail
