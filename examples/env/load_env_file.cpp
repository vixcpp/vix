/**
 *
 *  @file load_env_file.cpp
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
 */

#include <iostream>

#include <vix/env/Env.hpp>

int main()
{
  auto file = vix::env::load_file(".env");
  if (!file)
  {
    std::cerr << "load_file error: " << file.error().message() << '\n';
    return 1;
  }

  std::cout << "Loaded file: " << file.value().path << '\n';

  for (const auto &entry : file.value().entries)
  {
    std::cout << entry.key << " = " << entry.value << '\n';
  }

  return 0;
}
