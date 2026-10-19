#include <vix/Deadline.hpp>
#include <vix/Duration.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>

#include <cassert>
#include <array>
#include <cstddef>
#include <chrono>
#include <coroutine>
#include <exception>
#include <future>

namespace
{
  struct DetachedTask
  {
    struct promise_type
    {
      DetachedTask get_return_object() const noexcept { return {}; }
      std::suspend_never initial_suspend() const noexcept { return {}; }
      std::suspend_never final_suspend() const noexcept { return {}; }
      void return_void() const noexcept {}
      void unhandled_exception() const noexcept { std::terminate(); }
    };
  };

  DetachedTask probe_reentrant_run(vix::network::Provider &provider,
                                   vix::network::Endpoint endpoint,
                                   std::promise<bool> &observed)
  {
    auto connected = co_await provider.connect(endpoint);
    if (connected.status() != vix::CompletionStatus::Success)
    {
      observed.set_value(false);
      co_return;
    }
    auto connection = connected.take();
    (void)connection.close();
    auto nested = vix::network::run(provider, provider.connect(endpoint));
    observed.set_value(!nested &&
                       nested.error().identity() ==
                           vix::network::operation_in_progress);
  }

  vix::network::Endpoint loopback_ephemeral()
  {
    auto address = vix::network::Address::parse("127.0.0.1");
    assert(address);
    auto endpoint = vix::network::Endpoint::create(address.take(), 0);
    assert(endpoint);
    return endpoint.take();
  }

  void exercise(bool cooperative)
  {
    auto created = cooperative ? vix::network::Provider::create_cooperative()
                               : vix::network::Provider::create();
    assert(created);
    auto provider = created.take();

    auto listener_result = provider.listen(loopback_ephemeral());
    assert(listener_result);
    auto listener = listener_result.take();
    auto endpoint = listener.local_endpoint();
    assert(endpoint);

    // The prvalue operation is consumed without an application std::move.
    auto connected = vix::network::run(provider, provider.connect(endpoint.value()));
    assert(connected);
    auto connect_completion = connected.take();
    assert(connect_completion.status() == vix::CompletionStatus::Success);
    auto connection = connect_completion.take();

    auto accepted = vix::network::run(provider, listener.accept());
    assert(accepted);
    auto accept_completion = accepted.take();
    assert(accept_completion.status() == vix::CompletionStatus::Success);
    auto peer = accept_completion.take();

    std::array<std::byte, 0> empty_payload{};
    auto empty_send = vix::network::run(provider, connection.send(empty_payload));
    assert(empty_send);
    assert(empty_send.value().status() == vix::CompletionStatus::Success);

    std::promise<bool> reentrant_observed;
    auto reentrant_future = reentrant_observed.get_future();
    (void)probe_reentrant_run(provider, endpoint.value(), reentrant_observed);
    auto second_accept = vix::network::run(provider, listener.accept());
    assert(second_accept);
    auto second_accept_completion = second_accept.take();
    assert(second_accept_completion.status() == vix::CompletionStatus::Success);
    auto second_peer = second_accept_completion.take();
    if (cooperative)
    {
      for (unsigned turn = 0;
           turn < 8 && reentrant_future.wait_for(std::chrono::seconds(0)) !=
                           std::future_status::ready;
           ++turn)
        assert(provider.poll());
    }
    assert(reentrant_future.wait_for(std::chrono::seconds(2)) ==
           std::future_status::ready);
    assert(reentrant_future.get());
    assert(second_peer.close());

    assert(connection.close());
    assert(peer.close());
    assert(listener.close());

    vix::CancellationSource cancellation;
    (void)cancellation.request_cancel();
    auto cancelled = vix::network::run(
        provider,
        provider.connect(endpoint.value(), {cancellation.token(), {}}));
    assert(cancelled);
    assert(cancelled.value().status() == vix::CompletionStatus::Cancelled);

    auto invalid_address = vix::network::Address::parse("127.0.0.1");
    assert(invalid_address);
    auto invalid_endpoint =
        vix::network::Endpoint::create(invalid_address.take(), 0);
    assert(invalid_endpoint);
    auto rejected_operation =
        vix::network::run(provider, provider.connect(invalid_endpoint.value()));
    assert(rejected_operation);
    assert(rejected_operation.value().status() == vix::CompletionStatus::Failure);
    assert(rejected_operation.value().error().identity() ==
           vix::network::invalid_port);

    auto expired = vix::network::run(
        provider,
        provider.connect(endpoint.value(),
                         {{}, vix::Deadline::after(vix::Duration::zero())}));
    assert(expired);
    assert(expired.value().status() == vix::CompletionStatus::Failure);
    assert(expired.value().error().identity() == vix::network::timed_out);

    assert(provider.shutdown());
    auto stopped = vix::network::run(provider, provider.connect(endpoint.value()));
    assert(!stopped);
    assert(stopped.error().identity() == vix::network::provider_stopped);
  }
}

int main()
{
  exercise(false);
  exercise(true);
}
