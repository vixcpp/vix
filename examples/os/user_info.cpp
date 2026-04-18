/**
 *
 *  @file user_info.cpp
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
  auto user = vix::os::current_user();
  auto pid = vix::os::current_pid();
  auto admin = vix::os::is_admin();
  auto home = vix::os::home_dir();
  auto temp = vix::os::temp_dir();

  if (user)
  {
    std::cout << "Username  : " << user.value().username << '\n';
    std::cout << "Home dir  : " << user.value().home_dir << '\n';
    std::cout << "Shell     : " << user.value().shell << '\n';
  }
  else
  {
    std::cout << "User info : error: " << user.error().message() << '\n';
  }

  if (pid)
    std::cout << "PID       : " << pid.value() << '\n';
  else
    std::cout << "PID       : error: " << pid.error().message() << '\n';

  if (admin)
    std::cout << "Is admin  : " << (admin.value() ? "true" : "false") << '\n';
  else
    std::cout << "Is admin  : error: " << admin.error().message() << '\n';

  if (home)
    std::cout << "Home path : " << home.value() << '\n';
  else
    std::cout << "Home path : error: " << home.error().message() << '\n';

  if (temp)
    std::cout << "Temp path : " << temp.value() << '\n';
  else
    std::cout << "Temp path : error: " << temp.error().message() << '\n';

  return 0;
}
