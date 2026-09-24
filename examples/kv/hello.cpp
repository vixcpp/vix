/**
 *
 *  @file hello.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Small hello world example
 *
 */

#include <vix/kv/kv.hpp>

#include <iostream>

int main()
{
  auto kv = vix::kv::open("examples_data/hello");

  kv.put("hello", "world");

  const auto value = kv.get("hello");

  if (value.has_value())
  {
    std::cout << "hello = " << *value << '\n';
  }

  (void)kv.close();

  return 0;
}
