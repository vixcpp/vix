# Installation

This page explains how to add and build the Vix threadpool module.

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Requirements

The threadpool module requires:

- C++20
- CMake 3.20 or newer
- A compiler with standard threading support
- `pthread` on Linux-like systems

Supported compilers: GCC 11+, Clang 14+, Apple Clang with C++20 support.

## Repository layout

The module lives under `modules/threadpool/`:

```txt
modules/threadpool/
├── CMakeLists.txt
├── include/
│   └── vix/
│       └── threadpool/
│           └── all.hpp
├── src/
├── examples/
├── tests/
├── benchmarks/
└── docs/
```

## Using inside the Vix monorepo

When the module is inside the Vix monorepo, initialize it with:

```sh
git submodule update --init --recursive modules/threadpool
```

Then configure and build from the repository root:

```sh
cmake -S . -B build
cmake --build build
```

With Ninja:

```sh
cmake -S . -B build-ninja -G Ninja
cmake --build build-ninja
```

## Using as a standalone module

Clone the module:

```sh
git clone https://github.com/vixcpp/threadpool.git
cd threadpool
```

Configure and build:

```sh
cmake -S . -B build
cmake --build build
```

## Build examples

```sh
vix build --clean --preset dev -- -DVIX_THREADPOOL_BUILD_EXAMPLES=ON
./build-dev/examples/basic_post
```

Other available examples: `submit_future`, `task_priority`, `task_timeout`, `task_cancellation`, `task_group`, `parallel_for`, `parallel_for_each`, `parallel_map`, `parallel_reduce`, `periodic_task`, `metrics`, `shutdown`, `custom_config`.

## Build tests

```sh
vix build --clean --preset dev -- -DVIX_THREADPOOL_BUILD_TESTS=ON
ctest --test-dir build-dev --output-on-failure
```

Or run the test binary directly:

```sh
./build/tests/vix_threadpool_tests
```

## Build benchmarks

```sh
vix build --preset release --build-target submit_bench -- -DVIX_THREADPOOL_BUILD_BENCHMARKS=ON
./build-release/benchmarks/submit_bench
```

Available benchmarks: `submit_bench`, `parallel_for_bench`, `parallel_map_bench`, `queue_contention_bench`, `shutdown_bench`.

You can also build one benchmark target directly:

```bash
vix build --preset release -- -DVIX_THREADPOOL_BUILD_BENCHMARKS=ON --build-target submit_bench
./build-release/benchmarks/submit_bench
```

## Install with CMake

```sh
vix build --clean --preset release
cmake --install build-release
```

Install to a custom prefix:

```sh
vix build --clean --preset release -- -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --install build-release
```

Then consume it from another CMake project:

```cmake
find_package(vix-threadpool CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE vix::threadpool)
```

## Minimal CMake usage

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_threadpool_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(vix-threadpool CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE vix::threadpool)
```

`main.cpp`:

```cpp
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto future =
      pool.submit(
          []()
          {
            return 42;
          });

  std::cout << future.get() << '\n';

  pool.shutdown();

  return 0;
}
```

## Using with `add_subdirectory`

When the module is vendored into another project:

```txt
my_project/
├── CMakeLists.txt
├── main.cpp
└── third_party/
    └── threadpool/
```

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party/threadpool)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE vix::threadpool)
```

## Common build options

| Option | Default |
|---|---|
| `VIX_THREADPOOL_BUILD_EXAMPLES` | OFF |
| `VIX_THREADPOOL_BUILD_TESTS` | OFF |
| `VIX_THREADPOOL_BUILD_BENCHMARKS` | OFF |

Normal user build:

```sh
cmake -S . -B build \
  -DVIX_THREADPOOL_BUILD_EXAMPLES=OFF \
  -DVIX_THREADPOOL_BUILD_TESTS=OFF \
  -DVIX_THREADPOOL_BUILD_BENCHMARKS=OFF
```

Developer build:

```sh
cmake -S . -B build \
  -DVIX_THREADPOOL_BUILD_EXAMPLES=ON \
  -DVIX_THREADPOOL_BUILD_TESTS=ON \
  -DVIX_THREADPOOL_BUILD_BENCHMARKS=ON
```

## Linux notes

On Linux, the module uses standard C++ threads and may require `pthread` support. CMake links this automatically through `Threads::Threads`.

If you see thread-related linker errors, make sure your CMake links via the target, not just include directories:

```cmake
target_link_libraries(my_app PRIVATE vix::threadpool)
```

## Recommended include

Use the umbrella header for application code:

```cpp
#include <vix/threadpool.hpp>
```

Use direct headers only when you need smaller compile units:

```cpp
#include <vix/threadpool/ThreadPool.hpp>
#include <vix/threadpool/ParallelFor.hpp>
```

## Quick verification

Create `main.cpp`:

```cpp
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  auto future =
      pool.submit(
          []()
          {
            return 10 + 32;
          });

  std::cout << future.get() << '\n';

  pool.shutdown();

  return 0;
}
```

Build and run:

```sh
cmake -S . -B build
cmake --build build
./build/my_app
```

Expected output:

```txt
42
```
