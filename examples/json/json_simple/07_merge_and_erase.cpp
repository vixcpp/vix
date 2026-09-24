#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  kvs base = obj({"env", "prod",
                  "debug", false});

  kvs override = obj({"debug", true,
                      "trace", true});

  base.merge_from(override, true);

  base.erase("env");

  for (auto &k : base.keys())
    std::cout << "key=" << k << "\n";

  return 0;
}
