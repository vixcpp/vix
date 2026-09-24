#include <iostream>

#include <vix/webrpc/Router.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

int main()
{
  Router router;

  router.add("math.add", [](const Context &ctx) -> RpcResult
             {
    const auto *p = ctx.params_object_ptr();
    if (!p)
      return RpcError::invalid_params("params must be object");

    const auto a = p->get_i64_or("a", 0);
    const auto b = p->get_i64_or("b", 0);

    return obj({"sum", a + b}); });

  token req = obj({
      "id",
      42,
      "method",
      "math.add",
      "params",
      obj({"a", 7, "b", 5}),
  });

  RpcResult out = router.dispatch(req);

  if (std::holds_alternative<RpcError>(out))
  {
    std::cerr << "rpc error\n";
    return 1;
  }

  auto r = std::get<token>(out).as_object_ptr();
  std::cout << "sum = " << r->get_i64_or("sum", 0) << "\n";
  return 0;
}
