/**
 *
 *  @file quick_start.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 * @brief Demonstrates JSON object and array creation with Vix.cpp JSON builders.
 *
 * This example shows how to use the high-level helper functions:
 * - `o()` to build JSON objects using key/value pairs.
 * - `a()` to build JSON arrays.
 * - `dumps()` to pretty-print JSON with indentation.
 *
 * The Vix.cpp JSON builder syntax provides a clean, functional way
 * to construct JSON documents in C++ without verbose object creation.
 *
 * ### Example Output
 * ```
 * {
 *   "arr": [
 *     1,
 *     2,
 *     3
 *   ],
 *   "count": 3,
 *   "message": "Hello"
 * }
 * ```
 *
 * @see vix::json::o
 * @see vix::json::a
 * @see vix::json::dumps
 */

#include <vix/json/json.hpp>
#include <iostream>

int main()
{
  using namespace vix::json;

  // ---------------------------------------------------------------------
  // Build a JSON object and array using concise builder syntax
  // ---------------------------------------------------------------------
  auto j = o(
      "message", "Hello",
      "count", 3,
      "arr", a(1, 2, 3));

  // ---------------------------------------------------------------------
  // Pretty-print the JSON result
  // ---------------------------------------------------------------------
  std::cout << dumps(j, 2) << "\n";
}
