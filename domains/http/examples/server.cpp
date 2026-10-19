#include <vix/http/Http.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Provider.hpp>

int main()
{
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
  (void)request;
  (void)listener.close();
  (void)provider.shutdown();
}
