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

#include <vix/os/Os.hpp>

int main()
{
  auto platform = vix::os::platform();
  auto arch = vix::os::arch();
  auto hostname = vix::os::hostname();

  if (!platform)
  {
    std::cerr << "platform error: " << platform.error().message() << '\n';
    return 1;
  }

  if (!arch)
  {
    std::cerr << "arch error: " << arch.error().message() << '\n';
    return 1;
  }

  if (!hostname)
  {
    std::cerr << "hostname error: " << hostname.error().message() << '\n';
    return 1;
  }

  std::cout << "Platform : " << platform.value() << '\n';
  std::cout << "Arch     : " << arch.value() << '\n';
  std::cout << "Hostname : " << hostname.value() << '\n';

  return 0;
}
