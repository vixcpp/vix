#include <vix/http/Http.hpp>

#include <atomic>
#include <cstdlib>
#include <string>
#include <thread>

namespace
{
  std::vector<std::byte> bytes(std::string_view text)
  {
    std::vector<std::byte> out;
    for (const unsigned char c : text) out.push_back(static_cast<std::byte>(c));
    return out;
  }
  std::string text(const std::vector<std::byte> &data)
  {
    std::string out;
    for (const auto item : data)
      out.push_back(static_cast<char>(std::to_integer<unsigned char>(item)));
    return out;
  }
}

int main()
{
  auto provider_result = vix::network::Provider::create();
  if (!provider_result) return 1;
  auto provider = provider_result.take();
  auto address = vix::network::Address::parse("127.0.0.1");
  if (!address) return 2;
  auto bind_endpoint = vix::network::Endpoint::create(address.take(), 0);
  if (!bind_endpoint) return 3;
  auto listener_result = provider.listen(bind_endpoint.value());
  if (!listener_result) return 4;
  auto listener = listener_result.take();
  auto endpoint = listener.local_endpoint();
  if (!endpoint) return 5;

  std::atomic<bool> server_ok{};
  std::thread server([&]
  {
    auto prepared_accept = vix::http::accept(provider, listener);
    if (!prepared_accept) return;
    auto accepted = vix::network::run(provider, prepared_accept.take());
    if (!accepted || accepted.value().status() != vix::CompletionStatus::Success) return;
    auto session = accepted.take().take();
    auto prepared_request = session.receive_request();
    if (!prepared_request) return;
    auto request = vix::network::run(provider, prepared_request.take());
    if (!request || request.value().status() != vix::CompletionStatus::Success) return;
    if (request.value().value().target != "/installed") return;
    vix::http::Response response;
    response.body = bytes("OK");
    auto prepared_send = session.send_response(response);
    if (!prepared_send) return;
    auto sent = vix::network::run(provider, prepared_send.take());
    server_ok.store(sent && sent.value().status() == vix::CompletionStatus::Success,
                    std::memory_order_release);
  });

  vix::http::Request request;
  request.target = "/installed";
  request.headers.push_back({"Host", "localhost"});
  auto prepared = vix::http::exchange(provider, endpoint.value(), request);
  if (!prepared) { server.join(); return 6; }
  request.target = "/mutated";
  auto result = vix::network::run(provider, prepared.take());
  if (!result || result.value().status() != vix::CompletionStatus::Success) {
    server.join(); return 7;
  }
  const auto response = result.take().take();
  server.join();
  const bool passed = server_ok.load(std::memory_order_acquire) &&
                      response.status_code == 200 && text(response.body) == "OK";
  (void)listener.close();
  (void)provider.shutdown();
  return passed ? 0 : 8;
}
