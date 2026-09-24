/**
 *
 *  @file load_layered_env.cpp
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
  options.environment_name = "development";
  options.load_base_file = true;
  options.load_local_file = true;
  options.load_environment_file = true;
  options.load_environment_local_file = true;
  options.ignore_missing_files = true;

  auto files = vix::env::load_layered(options);
  if (!files)
  {
    std::cerr << "load_layered error: " << files.error().message() << '\n';
    return 1;
  }

  std::cout << "Loaded " << files.value().size() << " file(s)\n";

  for (const auto &file : files.value())
  {
    std::cout << "\n[" << file.path << "]\n";

    for (const auto &entry : file.entries)
    {
      std::cout << entry.key << " = " << entry.value << '\n';
    }
  }

  return 0;
}
