/**
 *
 *  @file persistent.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Durable persistence example
 *
 */

#include <vix/kv/kv.hpp>

#include <filesystem>
#include <iostream>
#include <string>

namespace
{
  namespace fs = std::filesystem;

  const fs::path data_path()
  {
    return fs::path{"examples_data"} / "persistent";
  }

  int fail(const std::string &message)
  {
    std::cerr << message << '\n';
    return 1;
  }

  int write_data()
  {
    auto kv = vix::kv::open(data_path());

    kv.put("users/1/name", "Ada");
    kv.put("users/2/name", "Grace");
    kv.put("settings/theme", "dark");

    auto flushed = kv.flush();

    if (flushed.is_err())
    {
      return fail(
          "failed to flush database: " +
          flushed.error().message());
    }

    std::cout << "write phase\n";
    std::cout << "-----------\n";
    std::cout << "users/1/name   = Ada\n";
    std::cout << "users/2/name   = Grace\n";
    std::cout << "settings/theme = dark\n";
    std::cout << "size           = " << kv.size() << "\n\n";

    auto closed = kv.close();

    if (closed.is_err())
    {
      return fail(
          "failed to close database after write: " +
          closed.error().message());
    }

    return 0;
  }

  int read_data_after_reopen()
  {
    auto kv = vix::kv::open(data_path());

    const auto user_1 = kv.get("users/1/name");
    const auto user_2 = kv.get("users/2/name");
    const auto theme = kv.get("settings/theme");

    if (!user_1.has_value())
    {
      return fail("failed to recover key: users/1/name");
    }

    if (!user_2.has_value())
    {
      return fail("failed to recover key: users/2/name");
    }

    if (!theme.has_value())
    {
      return fail("failed to recover key: settings/theme");
    }

    std::cout << "read phase after reopen\n";
    std::cout << "-----------------------\n";
    std::cout << "users/1/name   = " << *user_1 << '\n';
    std::cout << "users/2/name   = " << *user_2 << '\n';
    std::cout << "settings/theme = " << *theme << '\n';
    std::cout << "size           = " << kv.size() << '\n';

    const auto stats = kv.stats();

    std::cout << "wal recovered  = "
              << stats.wal_records_recovered
              << '\n';

    std::cout << "last sequence  = "
              << stats.last_sequence
              << "\n\n";

    auto closed = kv.close();

    if (closed.is_err())
    {
      return fail(
          "failed to close database after reopen: " +
          closed.error().message());
    }

    return 0;
  }

  int delete_and_reopen()
  {
    {
      auto kv = vix::kv::open(data_path());

      auto erased = kv.erase(
          vix::kv::KeyPath{"users", "1", "name"});

      if (erased.is_err())
      {
        return fail(
            "failed to erase users/1/name: " +
            erased.error().message());
      }

      auto flushed = kv.flush();

      if (flushed.is_err())
      {
        return fail(
            "failed to flush delete: " +
            flushed.error().message());
      }

      auto closed = kv.close();

      if (closed.is_err())
      {
        return fail(
            "failed to close database after delete: " +
            closed.error().message());
      }
    }

    {
      auto kv = vix::kv::open(data_path());

      const auto user_1 = kv.get("users/1/name");
      const auto user_2 = kv.get("users/2/name");

      if (user_1.has_value())
      {
        return fail(
            "deleted key should not be recovered: users/1/name");
      }

      if (!user_2.has_value())
      {
        return fail(
            "existing key should still be recovered: users/2/name");
      }

      std::cout << "delete phase after reopen\n";
      std::cout << "-------------------------\n";
      std::cout << "users/1/name   = deleted\n";
      std::cout << "users/2/name   = " << *user_2 << '\n';
      std::cout << "size           = " << kv.size() << '\n';

      const auto stats = kv.stats();

      std::cout << "tombstones     = "
                << stats.tombstone_count
                << '\n';

      auto closed = kv.close();

      if (closed.is_err())
      {
        return fail(
            "failed to close database after delete recovery: " +
            closed.error().message());
      }
    }

    return 0;
  }
}

int main()
{
  fs::remove_all(data_path());

  const int write_result = write_data();

  if (write_result != 0)
  {
    return write_result;
  }

  const int read_result = read_data_after_reopen();

  if (read_result != 0)
  {
    return read_result;
  }

  const int delete_result = delete_and_reopen();

  if (delete_result != 0)
  {
    return delete_result;
  }

  std::cout << "persistent example completed\n";
  return 0;
}
