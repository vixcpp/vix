/**
 *
 *  @file sleep.cpp
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

#include <vix/os/Os.hpp>

int main()
{
  std::cout << "Sleeping for 500 ms...\n";
  vix::os::sleep_for_ms(500);

  std::cout << "Sleeping for 1 second...\n";
  vix::os::sleep_for_seconds(1);

  std::cout << "Done.\n";
  return 0;
}
