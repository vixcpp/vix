/**
 *
 *  @file builders.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 * @brief Example usage of the Vix.cpp JSON module.
 *
 * This example demonstrates how to use the high-level JSON helpers
 * provided by `vix/json/json.hpp`, including:
 * - `o()` — for building JSON objects from key/value pairs.
 * - `a()` — for building arrays.
 * - `kv()` — for constructing an object from initializer pairs.
 * - `dumps()` — for pretty-printing JSON to a string.
 *
 * These helpers wrap `nlohmann::json` to make JSON creation
 * and formatting much more expressive and concise.
 *
 * ### Example Output
 * ```
 * {
 *   "id": 42,
 *   "name": "Ada",
 *   "tags": [
 *     "pro",
 *     "admin"
 *   ]
 * }
 * {
 *   "host": "localhost",
 *   "port": 8080
 * }
 * ```
 *
 * @see vix::json::o
 * @see vix::json::a
 * @see vix::json::kv
 * @see vix::json::dumps
 */

#include <vix/json/json.hpp>
#include <iostream>

int main()
{
  using namespace vix::json;

  // ---------------------------------------------------------------------
  // Build a JSON object using `o()` and an array using `a()`
  // ---------------------------------------------------------------------
  auto user = o(
      "id", 42,
      "name", "Ada",
      "tags", a("pro", "admin"));

  // ---------------------------------------------------------------------
  // Build another JSON object using `kv()` (initializer list syntax)
  // ---------------------------------------------------------------------
  auto conf = kv({{"host", "localhost"},
                  {"port", 8080}});

  // ---------------------------------------------------------------------
  // Pretty-print both JSON objects with indentation
  // ---------------------------------------------------------------------
  std::cout << dumps(user, 2) << "\n"
            << dumps(conf, 2) << "\n";
}
