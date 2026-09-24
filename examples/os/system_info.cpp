/**
 *
 *  @file system_info.cpp
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
  auto kernel = vix::os::kernel_version();
  auto hostname = vix::os::hostname();
  auto cpu_count = vix::os::cpu_count();
  auto page_size = vix::os::page_size();
  auto uptime = vix::os::uptime();

  if (platform)
    std::cout << "Platform        : " << platform.value() << '\n';
  else
    std::cout << "Platform        : error: " << platform.error().message() << '\n';

  if (arch)
    std::cout << "Architecture    : " << arch.value() << '\n';
  else
    std::cout << "Architecture    : error: " << arch.error().message() << '\n';

  if (kernel)
    std::cout << "Kernel version  : " << kernel.value() << '\n';
  else
    std::cout << "Kernel version  : error: " << kernel.error().message() << '\n';

  if (hostname)
    std::cout << "Hostname        : " << hostname.value() << '\n';
  else
    std::cout << "Hostname        : error: " << hostname.error().message() << '\n';

  if (cpu_count)
    std::cout << "CPU count       : " << cpu_count.value() << '\n';
  else
    std::cout << "CPU count       : error: " << cpu_count.error().message() << '\n';

  if (page_size)
    std::cout << "Page size       : " << page_size.value() << " bytes\n";
  else
    std::cout << "Page size       : error: " << page_size.error().message() << '\n';

  if (uptime)
    std::cout << "Uptime          : " << uptime.value() << " seconds\n";
  else
    std::cout << "Uptime          : error: " << uptime.error().message() << '\n';

  return 0;
}
