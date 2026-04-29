/**
 *
 *  @file multi_prompt.cpp
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
  const std::string name = vix::input("Enter your name: ");
  const std::string age = vix::input("Enter your age: ");
  const std::string email = vix::input("Enter your email: ");

  vix::print();
  vix::print("--- User information ---");
  vix::print("Name:", name);
  vix::print("Age:", age);
  vix::print("Email:", email);

  return 0;
}
