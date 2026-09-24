## project-setup
# Project setup

This guide shows the simplest possible Vix project layout.

Everything is minimal. Everything is explicit. No hidden structure.

------------------------------------------------------------------------

## 1. Minimal structure

``` txt
my-vix-app/
  CMakeLists.txt
  main.cpp
```

That is enough.

------------------------------------------------------------------------

## 2. main.cpp example

All routes directly inside `main`.

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.json({"message", "Welcome to Vix"});
  });

  app.get("/users/{id}", [](Request& req, Response& res) {
    const std::string id = req.param("id", "0");

    if (id == "0")
    {
      res.status(404).json({"error", "User not found"});
      return;
    }

    res.json({
      "id", id,
      "name", "User#" + id
    });
  });

  app.get("/search", [](Request& req, Response& res) {
    const std::string q = req.query_value("q", "");
    const std::string page = req.query_value("page", "1");

    res.json({
      "query", q,
      "page", page
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## 3. Minimal CMakeLists.txt

``` cmake
cmake_minimum_required(VERSION 3.20)
project(my_vix_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(my_vix_app main.cpp)

# Link Vix according to your setup
# target_link_libraries(my_vix_app PRIVATE vix::vix)
```

------------------------------------------------------------------------

## 4. Build manually

``` bash
mkdir build
cd build
cmake ..
cmake --build .
```

Run:

``` bash
./my_vix_app
```

------------------------------------------------------------------------

## 5. Scaling later

When your project grows, you can:

-   Split routes into multiple files
-   Add helpers for parsing
-   Add middleware
-   Add database layers

But the core idea stays the same:

Routes are explicit. Handlers are simple lambdas. `main` starts the
runtime.

Nothing more.

