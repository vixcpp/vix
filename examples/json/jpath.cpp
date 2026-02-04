/**
 *
 *  @file jpath.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 * @brief Demonstrates JSON navigation and mutation with JPath helpers.
 *
 * This example showcases how to use `jset()` and `jget()` to manipulate
 * deeply nested JSON structures via a human-readable *JPath* syntax.
 *
 * The JPath system supports:
 * - Dot notation (`user.profile.name`)
 * - Array indexing (`user.langs[2]`)
 * - Quoted keys for special characters (`user["display.name"]`)
 *
 * Missing intermediate objects and arrays are automatically created by `jset()`.
 * Invalid paths return `nullptr` when accessed via `jget()`.
 *
 * ### Example Output
 * ```
 * cpp
 * {
 *   "user": {
 *     "display.name": "Ada L.",
 *     "langs": [
 *       null,
 *       null,
 *       "cpp"
 *     ],
 *     "profile": {
 *       "name": "Gaspard"
 *     }
 *   }
 * }
 * ```
 *
 * @see vix::json::jget
 * @see vix::json::jset
 * @see vix::json::tokenize_path
 * @see vix::json::dumps
 */

#include <vix/json/json.hpp>
#include <iostream>

int main()
{
  using namespace vix::json;

  // ---------------------------------------------------------------------
  // Start with an empty JSON object
  // ---------------------------------------------------------------------
  Json j = obj();

  // ---------------------------------------------------------------------
  // Use jset() to assign values via JPath
  // Automatically creates missing objects/arrays
  // ---------------------------------------------------------------------
  jset(j, "user.langs[2]", "cpp");              // -> [null, null, "cpp"]
  jset(j, "user.profile.name", "Gaspard");      // creates nested object
  jset(j, R"(user["display.name"])", "Ada L."); // handles quoted keys

  // ---------------------------------------------------------------------
  // Access nested values via jget()
  // ---------------------------------------------------------------------
  if (auto v = jget(j, "user.langs[2]"))
  {
    std::cout << v->get<std::string>() << "\n"; // cpp
  }

  // ---------------------------------------------------------------------
  // Pretty-print the resulting JSON
  // ---------------------------------------------------------------------
  std::cout << dumps(j, 2) << "\n";
}
