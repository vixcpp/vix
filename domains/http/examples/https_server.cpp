#include <vix/http/Https.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Provider.hpp>

#include <iostream>
#include <string_view>

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "usage: https_server <certificate-chain.pem> <private-key.pem>\n";
    return 1;
  }
  auto provider_result = vix::network::Provider::create();
  if (!provider_result) return 2;
  auto provider = provider_result.take();
  auto address = vix::network::Address::parse("127.0.0.1");
  if (!address) return 3;
  auto endpoint = vix::network::Endpoint::create(address.take(), 8443);
  if (!endpoint) return 4;
  auto listener_result = provider.listen(endpoint.value());
  if (!listener_result) return 5;
  auto listener = listener_result.take();

  vix::tls::TlsServerOptions tls_options;
  tls_options.certificate_chain_file = argv[1];
  tls_options.private_key_file = argv[2];
  auto tls = vix::tls::TlsServerConfig::create(tls_options);
  if (!tls)
  {
    std::cerr << tls.error().message() << '\n';
    return 6;
  }
  auto accept_prepared = vix::http::https_accept(provider, listener, tls.value());
  if (!accept_prepared) return 7;
  auto accepted = vix::network::run(provider, accept_prepared.take());
  if (!accepted || accepted.value().status() != vix::CompletionStatus::Success) return 8;
  auto session = accepted.take().take();
  auto request_prepared = session.receive_request();
  if (!request_prepared) return 9;
  auto received = vix::network::run(provider, request_prepared.take());
  if (!received || received.value().status() != vix::CompletionStatus::Success) return 10;
  const auto request = received.take().take();

  vix::http::Response response;
  response.status_code = 200;
  response.reason = "OK";
  response.headers.push_back({"Content-Type", "text/plain"});
  for (const unsigned char value : std::string_view{"OK"})
    response.body.push_back(static_cast<std::byte>(value));
  auto send_prepared = session.send_response(response);
  if (!send_prepared) return 11;
  auto sent = vix::network::run(provider, send_prepared.take());
  if (!sent || sent.value().status() != vix::CompletionStatus::Success) return 12;
  std::cout << request.method << ' ' << request.target << '\n';
  (void)listener.close();
  (void)provider.shutdown();
}
