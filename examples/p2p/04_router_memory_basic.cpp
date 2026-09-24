/**
 *
 *  @file 04_router_memory_basic.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
// Run:
//   vix run examples/p2p/04_router_memory_basic.cpp

#include <iostream>
#include <optional>
#include <string>

#include <vix/p2p/Router.hpp>

namespace
{
  void print_route(const std::string &target,
                   const std::optional<vix::p2p::Route> &route)
  {
    if (!route)
    {
      std::cout << target << " -> no route\n";
      return;
    }

    std::cout << target << " -> "
              << "next_hop=" << route->next_hop
              << ", via_relay=" << (route->via_relay ? "true" : "false")
              << ", ttl=" << static_cast<int>(route->ttl)
              << "\n";
  }

  void run_example()
  {
    using namespace vix::p2p;

    MemoryRouter router;

    router.upsert_route("node-b", Route{"edge-1", false, 8});
    router.upsert_route("node-c", Route{"relay-7", true, 4});

    print_route("node-b", router.resolve("node-b"));
    print_route("node-c", router.resolve("node-c"));
    print_route("node-z", router.resolve("node-z"));

    router.remove_route("node-c");
    print_route("node-c", router.resolve("node-c"));
  }
}

int main()
{
  run_example();
  return 0;
}
