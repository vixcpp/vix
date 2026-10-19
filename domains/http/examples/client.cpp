#include <vix/http/Http.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Provider.hpp>

#include <iostream>

int main()
{
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
  if (!prepared)
  {
    std::cerr << prepared.error().message() << '\n';
    return 4;
  }
  auto observed = vix::network::run(provider, prepared.take());
  if (!observed) return 5;
  auto completion = observed.take();
  if (completion.status() != vix::CompletionStatus::Success)
  {
    if (completion.status() == vix::CompletionStatus::Failure)
      std::cerr << completion.error().message() << '\n';
    return 6;
  }
  const auto response = completion.take();
  std::cout << response.status_code << '\n';
  (void)provider.shutdown();
}
