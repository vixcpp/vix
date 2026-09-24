/**
 *
 *  @file persistence_small.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Small persistence example
 *
 */

#include <vix/kv/kv.hpp>

#include <filesystem>
#include <iostream>

int main()
{
  const std::filesystem::path path =
      std::filesystem::path{"examples_data"} / "persistence_small";

  std::filesystem::remove_all(path);

  {
    auto kv = vix::kv::open(path);

    kv.put("name", "Ada");
    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto kv = vix::kv::open(path);

    const auto value = kv.get("name");

    if (!value.has_value())
    {
      std::cerr << "failed to recover key: name\n";
      return 1;
    }

    std::cout << "name = " << *value << '\n';

    (void)kv.close();
  }

  return 0;
}
