#include <iostream>

#include <vix/webrpc/Request.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

int main()
{
  token raw = obj({
      "id",
      1,
      "method",
      "ping",
      "params",
      obj({"msg", "hello"}),
  });

  auto parsed = RpcRequest::parse(raw);

  if (std::holds_alternative<RpcError>(parsed))
  {
    std::cerr << "parse error\n";
    return 1;
  }

  const RpcRequest &req = std::get<RpcRequest>(parsed);

  std::cout << "method = " << req.method << "\n";
  std::cout << "has id = " << req.has_id() << "\n";

  if (auto p = req.params_object_ptr())
    std::cout << "msg = " << p->get_string_or("msg", "") << "\n";

  return 0;
}
