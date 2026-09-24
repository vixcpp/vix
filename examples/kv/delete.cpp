/**
 *
 *  @file delete.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Small delete example
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

  (void)kv.set({"session", "token"}, "abc123");

  std::cout << "before erase: "
            << (kv.contains({"session", "token"}) ? "exists" : "missing")
            << '\n';

  auto erased = kv.erase(vix::kv::KeyPath{"session", "token"});

  if (erased.is_err())
  {
    std::cerr << erased.error().message() << '\n';
    return 1;
  }

  std::cout << "after erase : "
            << (kv.contains({"session", "token"}) ? "exists" : "missing")
            << '\n';

  (void)kv.close();

  return 0;
}
