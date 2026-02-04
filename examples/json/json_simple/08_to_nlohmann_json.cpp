/**
 *
 *  @file 08_to_nlohmann_json.cpp
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

#include <vix/json/Simple.hpp>
#include <vix/json/convert.hpp> // to_json(Simple) -> Json
#include <vix/json/json.hpp>    // vix::json::Json

namespace
{
  void example_basic_object()
  {
    using namespace vix::json;

    kvs user = obj({
        "name",
        "Alice",
        "age",
        30,
        "ok",
        true,
        "score",
        12.5,
        "skills",
        array({"C++", "P2P", "Offline-first"}),
    });

    Json j = to_json(user);

    std::cout << "\n[example_basic_object]\n";
    std::cout << j.dump(2) << "\n";
  }

  void example_nested()
  {
    using namespace vix::json;

    kvs root = obj({
        "app",
        "Vix.cpp",
        "meta",
        obj({
            "country",
            "UG",
            "version",
            "1.0.0",
            "features",
            array({"http", "ws", "p2p"}),
        }),
        "users",
        array({
            obj({"id", 1, "name", "Ada"}),
            obj({"id", 2, "name", "Gaspard"}),
        }),
    });

    token t = root; // token root holding object

    Json j = to_json(t);

    std::cout << "\n[example_nested]\n";
    std::cout << j.dump(2) << "\n";
  }

  void example_roundtrip_style_usage()
  {
    using namespace vix::json;

    kvs payload = obj({
        "type",
        "notification",
        "data",
        obj({
            "title",
            "Build OK",
            "code",
            200,
            "tags",
            array({"ci", "release", "vix"}),
        }),
    });

    Json j = to_json(payload);

    std::cout << "\n[example_roundtrip_style_usage]\n";
    std::cout << "type=" << j.value("type", "missing") << "\n";
    std::cout << j.dump(2) << "\n";
  }

} // namespace

int main()
{
  example_basic_object();
  example_nested();
  example_roundtrip_style_usage();
  return 0;
}
