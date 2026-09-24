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

#include <vix/log/Log.hpp>

int main()
{
  vix::log::set_level(vix::log::LogLevel::Trace);

  vix::log::trace("trace message");
  vix::log::debug("debug message");
  vix::log::info("info message");
  vix::log::warn("warn message");
  vix::log::error("error message");
  vix::log::critical("critical message");

  return 0;
}
