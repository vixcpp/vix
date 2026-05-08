/**
 *
 *  @file stats.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Small stats example
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

  (void)kv.set({"a"}, "one");
  (void)kv.set({"b"}, "two");
  (void)kv.erase(vix::kv::KeyPath{"a"});

  const auto stats = kv.stats();

  std::cout << "open          = "
            << (stats.open ? "yes" : "no")
            << '\n';

  std::cout << "memory only   = "
            << (stats.memory_only ? "yes" : "no")
            << '\n';

  std::cout << "live keys     = "
            << stats.key_count
            << '\n';

  std::cout << "tombstones    = "
            << stats.tombstone_count
            << '\n';

  std::cout << "last sequence = "
            << stats.last_sequence
            << '\n';

  (void)kv.close();

  return 0;
}
