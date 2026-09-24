/**
 *
 *  @file basic.cpp
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
  auto app_name = vix::env::get_or("APP_NAME", "Vix");
  auto app_env = vix::env::get_or("APP_ENV", "development");

  std::cout << "APP_NAME = " << app_name << '\n';
  std::cout << "APP_ENV  = " << app_env << '\n';

  if (vix::env::has("PORT"))
  {
    auto port = vix::env::get("PORT");
    if (port)
    {
      std::cout << "PORT     = " << port.value() << '\n';
    }
  }

  return 0;
}
