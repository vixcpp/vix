/**
 *
 *  @file format.cpp
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
  vix::log::set_level(vix::log::LogLevel::Info);

  vix::log::set_format(vix::log::LogFormat::KV);
  vix::log::logf(
      vix::log::LogLevel::Info,
      "kv example",
      "status", 200,
      "method", "GET");

  vix::log::set_format(vix::log::LogFormat::JSON);
  vix::log::logf(
      vix::log::LogLevel::Info,
      "json example",
      "status", 201,
      "method", "POST");

  vix::log::set_format(vix::log::LogFormat::JSON_PRETTY);
  vix::log::logf(
      vix::log::LogLevel::Info,
      "pretty json example",
      "status", 500,
      "method", "DELETE");

  return 0;
}
