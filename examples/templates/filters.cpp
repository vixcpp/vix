/**
 *
 *  @file filters.cpp
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
 *
 */
#include <iostream>
#include <string>
#include <unordered_map>

#include <vix/template/Builtins.hpp>
#include <vix/template/Value.hpp>

using namespace vix::template_;

int main()
{
  const std::unordered_map<std::string, Filter> filters = Builtins::filters();

  const Value name("Gaspard");
  const Value items(Array{Value("Laptop"), Value("Phone"), Value("Book")});
  const Value empty("");

  const Value upper_name = filters.at("upper")(name);
  const Value lower_name = filters.at("lower")(name);
  const Value items_length = filters.at("length")(items);
  const Value default_empty = filters.at("default")(empty);

  std::cout << "upper(name): " << upper_name.to_string() << '\n';
  std::cout << "lower(name): " << lower_name.to_string() << '\n';
  std::cout << "length(items): " << items_length.to_string() << '\n';
  std::cout << "default(empty): [" << default_empty.to_string() << "]\n";

  return 0;
}
