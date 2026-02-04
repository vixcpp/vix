/**
 *
 *  @file io.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 * @brief Demonstrates parsing, writing, and reloading JSON with Vix.cpp helpers.
 *
 * This example shows how to:
 * - Parse a JSON string with `loads()`.
 * - Write it safely to disk using `dump_file()` (atomic operation).
 * - Reload the same file using `load_file()`.
 * - Print the formatted JSON using `dumps()`.
 *
 * The `dump_file()` function writes to a temporary file (with `.tmp` suffix)
 * before renaming it atomically to prevent corruption — making it suitable
 * for configuration storage, caching, or transactional file operations.
 *
 * ### Example Output
 * ```
 * {
 *   "a": 1,
 *   "b": [
 *     10,
 *     20
 *   ]
 * }
 * ```
 *
 * @see vix::json::loads
 * @see vix::json::dump_file
 * @see vix::json::load_file
 * @see vix::json::dumps
 */

#include <vix/json/json.hpp>
#include <iostream>

int main()
{
  using namespace vix::json;

  // ---------------------------------------------------------------------
  // Parse a JSON string into a Json object
  // ---------------------------------------------------------------------
  auto j = loads(R"({"a":1,"b":[10,20]})");

  // ---------------------------------------------------------------------
  // Write JSON to disk safely (atomic write using .tmp)
  // ---------------------------------------------------------------------
  dump_file("out.json", j, 2);

  // ---------------------------------------------------------------------
  // Read the same file back into a new Json object
  // ---------------------------------------------------------------------
  auto j2 = load_file("out.json");

  // ---------------------------------------------------------------------
  // Pretty-print the reloaded JSON to stdout
  // ---------------------------------------------------------------------
  std::cout << dumps(j2, 2) << "\n";
}
