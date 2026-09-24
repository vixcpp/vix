/**
 *
 *  @file list.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Prefix listing example
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

  void print_key(const vix::kv::KeyPath &key)
  {
    for (std::size_t index = 0; index < key.size(); ++index)
    {
      if (index > 0)
      {
        std::cout << "/";
      }

      std::cout << key.at(index);
    }
  }

  void print_entries(
      const std::string &title,
      const vix::kv::Kv::ListResult &entries)
  {
    std::cout << title << '\n';
    std::cout << std::string(title.size(), '-') << '\n';

    if (entries.empty())
    {
      std::cout << "no entries\n\n";
      return;
    }

    for (const auto &[key, value] : entries)
    {
      std::cout << "key   : ";
      print_key(key);
      std::cout << '\n';

      std::cout << "value : "
                << value.to_string()
                << "\n\n";
    }
  }

  int seed_data(vix::kv::Kv &kv)
  {
    auto user_1 = kv.set(
        {"users", "1", "name"},
        "Ada");

    if (user_1.is_err())
    {
      return fail(
          "failed to store users/1/name: " +
          user_1.error().message());
    }

    auto user_2 = kv.set(
        {"users", "2", "name"},
        "Grace");

    if (user_2.is_err())
    {
      return fail(
          "failed to store users/2/name: " +
          user_2.error().message());
    }

    auto user_3 = kv.set(
        {"users", "3", "name"},
        "Linus");

    if (user_3.is_err())
    {
      return fail(
          "failed to store users/3/name: " +
          user_3.error().message());
    }

    auto setting = kv.set(
        {"settings", "theme"},
        "dark");

    if (setting.is_err())
    {
      return fail(
          "failed to store settings/theme: " +
          setting.error().message());
    }

    auto cache = kv.set(
        {"cache", "version"},
        "1");

    if (cache.is_err())
    {
      return fail(
          "failed to store cache/version: " +
          cache.error().message());
    }

    auto erased = kv.erase(
        vix::kv::KeyPath{"users", "3", "name"});

    if (erased.is_err())
    {
      return fail(
          "failed to erase users/3/name: " +
          erased.error().message());
    }

    return 0;
  }

  int run_list_example()
  {
    auto opened = vix::kv::open_memory();

    if (opened.is_err())
    {
      return fail(
          "failed to open memory database: " +
          opened.error().message());
    }

    auto kv = opened.move_value();

    const int seed_result = seed_data(kv);

    if (seed_result != 0)
    {
      (void)kv.close();
      return seed_result;
    }

    auto users = kv.list({"users"});

    if (users.is_err())
    {
      (void)kv.close();

      return fail(
          "failed to list users prefix: " +
          users.error().message());
    }

    auto settings = kv.list({"settings"});

    if (settings.is_err())
    {
      (void)kv.close();

      return fail(
          "failed to list settings prefix: " +
          settings.error().message());
    }

    auto all = kv.list();

    if (all.is_err())
    {
      (void)kv.close();

      return fail(
          "failed to list all entries: " +
          all.error().message());
    }

    print_entries(
        "users prefix",
        users.value());

    print_entries(
        "settings prefix",
        settings.value());

    print_entries(
        "all live entries",
        all.value());

    std::cout << "size       : "
              << kv.size()
              << '\n';

    const auto stats = kv.stats();

    std::cout << "tombstones : "
              << stats.tombstone_count
              << '\n';

    auto closed = kv.close();

    if (closed.is_err())
    {
      return fail(
          "failed to close database: " +
          closed.error().message());
    }

    return 0;
  }
}

int main()
{
  const int result = run_list_example();

  if (result != 0)
  {
    return result;
  }

  std::cout << "list example completed\n";
  return 0;
}
