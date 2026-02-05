#include <iostream>

#include <vix/webrpc/Dispatcher.hpp>
#include <vix/webrpc/Router.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

int main()
{
  Router router;

  router.add("log", [](const Context &ctx) -> RpcResult
             {
    if (const auto *p = ctx.params_object_ptr())
      std::cout << "log: " << p->get_string_or("msg", "") << "\n";
    return token(nullptr); });

  Dispatcher d(router);

  token notification = obj({
      "method",
      "log",
      "params",
      obj({"msg", "fire and forget"}),
  });

  auto res = d.handle(notification);
  if (!res.has_value())
    std::cout << "no response (notification)\n";

  return 0;
}
