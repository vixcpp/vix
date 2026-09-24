/**
 *
 *  @file production_env.cpp
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
  vix::env::EnvFileOptions options;
  options.mode = vix::env::EnvFileMode::Layered;
  options.base_dir = ".";
  options.filename = ".env";
  options.environment_name = "production";
  options.load_base_file = true;
  options.load_local_file = false;
  options.load_environment_file = true;
  options.load_environment_local_file = true;
  options.ignore_missing_files = true;
  options.env.overwrite_existing = true;

  auto err = vix::env::load_layered_into_process(options);
  if (err)
  {
    std::cerr << "load_layered_into_process error: " << err.message() << '\n';
    return 1;
  }

  std::cout << "Production environment loaded into process\n";

  auto app_env = vix::env::get_or("APP_ENV", "unknown");
  auto app_name = vix::env::get_or("APP_NAME", "unnamed");

  std::cout << "APP_NAME = " << app_name << '\n';
  std::cout << "APP_ENV  = " << app_env << '\n';

  auto debug = vix::env::get_bool("APP_DEBUG");
  if (debug)
  {
    std::cout << "APP_DEBUG = " << (debug.value() ? "true" : "false") << '\n';
  }

  return 0;
}
