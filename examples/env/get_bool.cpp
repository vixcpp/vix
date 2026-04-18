/**
 *
 *  @file get_bool.cpp
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
  auto debug = vix::env::get_bool("APP_DEBUG");
  if (!debug)
  {
    std::cerr << "APP_DEBUG error: " << debug.error().message() << '\n';
    return 1;
  }

  std::cout << "APP_DEBUG = "
            << (debug.value() ? "true" : "false")
            << '\n';

  return 0;
}
