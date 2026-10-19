# Network top-level operation runner (experimental)

`vix::network::run(provider, operation)` lets a synchronous C++ entry point
start one existing `Operation<T>` and receive its `Completion<T>` without
declaring a coroutine task. The argument is an rvalue reference, so a returned
operation can be passed directly; ordinary use does not spell `std::move`.
The runner blocks the calling thread until terminal completion.

The return type is `vix::Result<vix::Completion<T>>`. The outer `Result`
reports that the runner could not start (for example, the provider is stopped
or the call would be reentrant). The inner `Completion<T>` is the operation's
normal outcome: success, failure, cancellation, or deadline expiry. A failure
reported by the operation is not converted into a runner failure.

Background providers continue on their existing reactor and continuation
workers while the caller waits on a condition variable. Cooperative providers
are driven on their owner thread using a blocking native-reactor wait followed
by the existing provider progress turn. This does not busy-poll and creates no
additional thread, scheduler, or reactor. Consequently, the cooperative form
blocks its owner thread and is not appropriate inside a provider callback.
Calls on provider progress threads are rejected before the operation starts.
The Provider object must remain alive until `run` returns.

Example: perform and observe a local TCP connect from `main`:

```cpp
#include <vix/network/Address.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>

int main()
{
  auto provider_result = vix::network::Provider::create();
  if (!provider_result) return 1;
  auto provider = provider_result.take();

  auto address = vix::network::Address::parse("127.0.0.1");
  if (!address) return 2;
  auto requested = vix::network::Endpoint::create(address.take(), 0);
  if (!requested) return 3;
  auto listener_result = provider.listen(requested.value());
  if (!listener_result) return 4;
  auto listener = listener_result.take();
  auto endpoint = listener.local_endpoint();
  if (!endpoint) return 5;

  auto run_result = vix::network::run(provider, provider.connect(endpoint.value()));
  if (!run_result) return 6; // runner/provider failure, before operation result
  auto completion = run_result.take();
  if (completion.status() != vix::CompletionStatus::Success) return 7;
  auto connection = completion.take();
  (void)connection.close();
  (void)listener.close();
  return provider.shutdown() ? 0 : 8;
}
```

Example: distinguish an accepted operation's timeout from runner failure:

```cpp
#include <vix/Deadline.hpp>
#include <vix/Duration.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>

int main()
{
  auto provider_result = vix::network::Provider::create();
  if (!provider_result) return 1;
  auto provider = provider_result.take();
  auto address = vix::network::Address::parse("127.0.0.1");
  if (!address) return 2;
  auto endpoint = vix::network::Endpoint::create(address.take(), 9);
  if (!endpoint) return 3;

  auto run_result = vix::network::run(
      provider,
      provider.connect(endpoint.value(),
          {{}, vix::Deadline::after(vix::Duration::zero())}));
  if (!run_result) return 4; // inability to enter the runner
  auto completion = run_result.take();
  if (completion.status() == vix::CompletionStatus::Cancelled) return 5;
  if (completion.status() == vix::CompletionStatus::Failure &&
      completion.error().identity() == vix::network::timed_out)
    return provider.shutdown() ? 0 : 6;
  return 7;
}
```

`run` is a top-level bridge only. Existing `co_await` composition and the
incomplete-awaiter lifetime contract are unchanged. It does not let callers
abandon active operations or synchronously interrupt arbitrary work.
