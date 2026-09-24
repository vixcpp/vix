#include <iostream>

#include <vix/webrpc/Dispatcher.hpp>
#include <vix/webrpc/Router.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

int main()
{
  Router router;

  router.add("echo", [](const Context &ctx) -> RpcResult
             { return ctx.params; });

  Dispatcher d(router);

  token batch = array({
      obj({"id", 1, "method", "echo", "params", obj({"x", 10})}),
      obj({"method", "echo", "params", obj({"y", 20})}), // notification
      obj({"id", 2, "method", "echo", "params", obj({"z", 30})}),
  });

  auto res = d.handle(batch);

  if (!res)
  {
    std::cout << "no responses\n";
    return 0;
  }

  auto arr = res->as_array_ptr();
  std::cout << "responses = " << arr->size() << "\n";
  return 0;
}
