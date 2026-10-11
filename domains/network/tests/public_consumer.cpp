#include <vix/network/Provider.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>

#include <type_traits>
#include <array>

static void transport_spelling(vix::network::TcpConnection &connection)
{
  std::array<std::byte, 16> bytes{};
  auto receive=connection.receive(bytes);
  auto send=connection.send(bytes);
  auto shutdown=connection.shutdown_write();
  (void)receive;
  (void)send;
  (void)shutdown;
}
static void listener_spelling(vix::network::Provider &provider, const vix::network::Endpoint &endpoint)
{
  auto listener_result=provider.listen(endpoint);
  if (!listener_result) return;
  auto listener=listener_result.take();
  auto local=listener.local_endpoint();
  auto accept=listener.accept();
  auto closed=listener.close();
  (void)local;
  (void)accept;
  (void)closed;
}

int main()
{
  static_assert(!std::is_copy_constructible_v<vix::network::Provider>);
  auto provider_result=vix::network::Provider::create();
  if (!provider_result) return 1;
  auto provider=provider_result.take();

  auto address_result=vix::network::Address::parse("127.0.0.1");
  if (!address_result) return 1;
  auto address=address_result.take();

  auto endpoint_result=vix::network::Endpoint::create(address, 8080);
  if (!endpoint_result) return 1;
  auto endpoint=endpoint_result.take();

  auto operation=provider.connect(endpoint);
  (void)operation;
  (void)&transport_spelling;
  (void)&listener_spelling;
  return provider.shutdown() ? 0 : 1;
}
