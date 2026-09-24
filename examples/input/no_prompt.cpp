/**
 *
 *  @file no_prompt.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */

#include <vix/input.hpp>
#include <vix/print.hpp>

int main()
{
  vix::print("Type something, then press Enter:");

  const std::string value = vix::input();

  vix::print("You entered:", value);

  return 0;
}
