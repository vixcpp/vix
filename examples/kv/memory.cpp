/**
 *
 *  @file memory.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Small memory-only example
 *
 */

#include <vix/kv/kv.hpp>

#include <iostream>

int main()
{
  auto opened = vix::kv::open_memory();

  if (opened.is_err())
  {
    std::cerr << opened.error().message() << '\n';
    return 1;
  }

  auto kv = opened.move_value();

  auto written = kv.set({"users", "1", "name"}, "Ada");

  if (written.is_err())
  {
    std::cerr << written.error().message() << '\n';
    return 1;
  }

  auto value = kv.get({"users", "1", "name"});

  if (value.is_ok())
  {
    std::cout << "users/1/name = "
              << value.value().to_string()
              << '\n';
  }

  (void)kv.close();

  return 0;
}
