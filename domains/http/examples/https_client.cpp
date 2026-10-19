#include <vix/http/Https.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Provider.hpp>
#include <vix/tls/Errors.hpp>

#include <iostream>

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "usage: https_client <trust-root.pem>\n";
    return 1;
  }
  auto provider_result = vix::network::Provider::create();
  if (!provider_result) return 2;
  auto provider = provider_result.take();
  auto address = vix::network::Address::parse("127.0.0.1");
  if (!address) return 3;
  auto endpoint = vix::network::Endpoint::create(address.take(), 8443);
  if (!endpoint) return 4;

  vix::tls::TlsClientOptions tls_options;
  tls_options.expected_peer = {vix::tls::PeerIdentityKind::dns_name, "localhost"};
  tls_options.ca_file = argv[1];
  tls_options.use_system_trust = false;
  auto tls = vix::tls::TlsClientConfig::create(tls_options);
  if (!tls)
  {
    std::cerr << tls.error().message() << '\n';
    return 5;
  }

  vix::http::Request request;
  request.target = "/health";
  request.headers.push_back({"Host", "localhost"});
  auto prepared = vix::http::https_exchange(provider, endpoint.value(), request, tls.value());
  if (!prepared)
  {
    std::cerr << prepared.error().message() << '\n';
    return 6;
  }
  auto observed = vix::network::run(provider, prepared.take());
  if (!observed) return 7;
  auto completion = observed.take();
  if (completion.status() != vix::CompletionStatus::Success)
  {
    if (completion.status() == vix::CompletionStatus::Failure)
      std::cerr << completion.error().message() << '\n';
    return 8;
  }
  std::cout << completion.take().status_code << '\n';
  (void)provider.shutdown();
}
