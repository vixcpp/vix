#include <vix/concurrency/ExecutionResource.hpp>
#include <vix/network/Provider.hpp>

int main()
{
#if defined(__wasi__)
  auto execution = vix::concurrency::ExecutionResource::create();
  if (execution || execution.error().identity() != vix::concurrency::errors::execution_unavailable)
    return 1;
  auto provider = vix::network::Provider::create_cooperative();
#else
  auto execution = vix::concurrency::ExecutionResource::create({1, 0});
  if (!execution)
    return 2;
  auto provider = vix::network::Provider::create();
#endif
  if (!provider)
    return 3;
  return provider.value().shutdown() ? 0 : 4;
}
