/**
 *
 *  @file basic.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Basic public API example
 *
 */

#include <vix/kv/kv.hpp>

#include <iostream>
#include <string>

namespace
{
  int fail(const std::string &message)
  {
    std::cerr << message << '\n';
    return 1;
  }

  int run_direct_api_example()
  {
    auto kv = vix::kv::open("examples_data/basic");

    kv.put("hello", "world");
    kv.put("users/1/name", "Ada");

    const auto hello = kv.get("hello");
    const auto user_name = kv.get("users/1/name");

    if (!hello.has_value())
    {
      return fail("failed to read key: hello");
    }

    if (!user_name.has_value())
    {
      return fail("failed to read key: users/1/name");
    }

    std::cout << "direct API\n";
    std::cout << "----------\n";
    std::cout << "hello          = " << *hello << '\n';
    std::cout << "users/1/name   = " << *user_name << '\n';
    std::cout << "size           = " << kv.size() << "\n\n";

    auto closed = kv.close();

    if (closed.is_err())
    {
      return fail(
          "failed to close direct API database: " +
          closed.error().message());
    }

    return 0;
  }

  int run_result_api_example()
  {
    auto opened = vix::kv::open_memory();

    if (opened.is_err())
    {
      return fail(
          "failed to open memory database: " +
          opened.error().message());
    }

    auto kv = opened.move_value();

    auto written = kv.set(
        {"users", "1", "email"},
        "ada@example.com");

    if (written.is_err())
    {
      return fail(
          "failed to write structured key: " +
          written.error().message());
    }

    auto value = kv.get({"users", "1", "email"});

    if (value.is_err())
    {
      return fail(
          "failed to read structured key: " +
          value.error().message());
    }

    std::cout << "result API\n";
    std::cout << "----------\n";
    std::cout << "users/1/email = "
              << value.value().to_string()
              << '\n';

    std::cout << "contains       = "
              << (kv.contains({"users", "1", "email"}) ? "yes" : "no")
              << '\n';

    std::cout << "size           = "
              << kv.size()
              << "\n\n";

    auto closed = kv.close();

    if (closed.is_err())
    {
      return fail(
          "failed to close result API database: " +
          closed.error().message());
    }

    return 0;
  }
}

int main()
{
  const int direct_result = run_direct_api_example();

  if (direct_result != 0)
  {
    return direct_result;
  }

  const int result_api_result = run_result_api_example();

  if (result_api_result != 0)
  {
    return result_api_result;
  }

  std::cout << "basic example completed\n";
  return 0;
}
