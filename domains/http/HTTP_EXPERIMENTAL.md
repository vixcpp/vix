# Experimental HTTP/1.1 API

This Linux-only opt-in API is provisional. Configure with
`VIX_BUILD_HTTP_EXPERIMENTAL=ON` and `VIX_INSTALL_HTTP_EXPERIMENTAL=ON` to
build and install the `HTTPExperimental` package component. It uses numeric
Network endpoints, one connection per exchange/session, buffered messages
limited to 64 KiB bodies and 8 KiB serialized outgoing messages, and a
30-second absolute operation budget. It has no DNS, streaming, persistence,
pooling, retry, redirect, HTTPS, or upgrade support. Incoming trailers are
represented; outgoing trailers and caller-provided Content-Length,
Transfer-Encoding, or Connection are rejected because Vix generates framing
and emits `Connection: close` for the one-shot transport.

`exchange` and `accept` return `Result<network::Operation<T>>`. This outer
Result reports synchronous preparation/allocation failures. After taking the
operation, `network::run` returns `Result<Completion<T>>`: its Result reports
runner admission failure, while Completion is the operation's terminal
success, failure, or cancellation. `network::run` blocks the calling thread.
Do not block sequentially on a cooperative provider when two peer operations
must progress; progress both from the provider's owner thread, or use separate
threads only with a background provider.

## Client

```cpp
#include <vix/http/Http.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Provider.hpp>
#include <iostream>

int main() {
  auto provider_result = vix::network::Provider::create();
  if (!provider_result) return 1;
  auto provider = provider_result.take();
  auto address_result = vix::network::Address::parse("127.0.0.1");
  if (!address_result) return 2;
  auto endpoint_result = vix::network::Endpoint::create(address_result.take(), 8080);
  if (!endpoint_result) return 3;

  vix::http::Request request;
  request.target = "/health";
  request.headers.push_back({"Host", "localhost"});
  auto prepared = vix::http::exchange(provider, endpoint_result.value(), request);
  if (!prepared) {
    std::cerr << prepared.error().message() << '\n';
    return 4;
  }
  auto observed = vix::network::run(provider, prepared.take());
  if (!observed) return 5; // runner admission, not HTTP terminal outcome
  auto completion = observed.take();
  if (completion.status() != vix::CompletionStatus::Success) {
    if (completion.status() == vix::CompletionStatus::Failure)
      std::cerr << completion.error().message() << '\n';
    return 6;
  }
  const auto response = completion.take();
  std::cout << response.status_code << '\n';
  (void)provider.shutdown();
}
```

## Server

```cpp
#include <vix/http/Http.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Provider.hpp>

int main() {
  auto provider_result = vix::network::Provider::create();
  if (!provider_result) return 1;
  auto provider = provider_result.take();
  auto address_result = vix::network::Address::parse("127.0.0.1");
  if (!address_result) return 2;
  auto endpoint_result = vix::network::Endpoint::create(address_result.take(), 8080);
  if (!endpoint_result) return 3;
  auto listener_result = provider.listen(endpoint_result.value());
  if (!listener_result) return 4;
  auto listener = listener_result.take();

  auto accept_prepared = vix::http::accept(provider, listener);
  if (!accept_prepared) return 5;
  auto accepted = vix::network::run(provider, accept_prepared.take());
  if (!accepted || accepted.value().status() != vix::CompletionStatus::Success) return 6;
  auto session = accepted.take().take();

  auto request_prepared = session.receive_request();
  if (!request_prepared) return 7;
  auto request_result = vix::network::run(provider, request_prepared.take());
  if (!request_result || request_result.value().status() != vix::CompletionStatus::Success) return 8;
  const auto request = request_result.take().take();

  vix::http::Response response;
  response.body = {static_cast<std::byte>('O'), static_cast<std::byte>('K')};
  response.headers.push_back({"Content-Type", "text/plain"});
  auto send_prepared = session.send_response(response);
  if (!send_prepared) return 9;
  auto sent = vix::network::run(provider, send_prepared.take());
  if (!sent || sent.value().status() != vix::CompletionStatus::Success) return 10;
  (void)request; // application policy belongs to the application
  (void)listener.close();
  (void)provider.shutdown();
}
```
