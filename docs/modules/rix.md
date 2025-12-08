<p align="center" style="margin:0;">
  <img 
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1764778802/logo_rix_gyfnhr.png" 
    alt="Rix Banner" 
    width="100%" 
    style="
      display:block;
      height:auto;
      max-width:900px;
      margin:auto;
      object-fit:cover;
      border-radius:8px;
    ">
</p>

<h1 align="center">Rix - Minimal Modern C++ Standard Library</h1>

<p align="center">
  <img src="https://img.shields.io/badge/C++20-Standard-blue">
  <img src="https://img.shields.io/badge/License-MIT-green">
</p>

# Rix is a lightweight and modern collection of essential utilities

designed for C++20 and beyond.\
It serves as a modular, header-only, minimal standard library, ideal
for:

- Modern C++ development
- Embedded / lightweight projects
- High-performance frameworks like Vix.cpp
- Developers who want a clean, consistent, simplified alternative to
  the STL

Each module is provided as a standalone repository, while `rix` (the
umbrella project) offers a unified interface for convenience.

---

## Modules

| Module           | Namespace         | Description                                                                                  |
| ---------------- | ----------------- | -------------------------------------------------------------------------------------------- |
| `rix-io`         | `rix::io`         | File and I/O utilities: reading and writing text and binary data.                            |
| `rix-memory`     | `rix::memory`     | Memory management utilities, smart pointers, and RAII helpers.                               |
| `rix-containers` | `rix::containers` | Core containers: vector, map, set implementations using modern C++.                          |
| `rix-algo`       | `rix::algo`       | Common algorithms: sorting, searching, transformations.                                      |
| `rix-string`     | `rix::string`     | String manipulation utilities and helper functions.                                          |
| `rix-fs`         | `rix::fs`         | Filesystem utilities: paths, directories, file operations.                                   |
| `rix-net`        | `rix::net`        | TCP/UDP client and server abstractions.                                                      |
| `rix-json`       | `rix::json`       | JSON parser and serializer for C++20.                                                        |
| `rix-chrono`     | `rix::chrono`     | Timers, sleep functions, and time measurement utilities.                                     |
| `rix-log`        | `rix::log`        | Logging utilities: console and file output.                                                  |
| `rix-process`    | `rix::process`    | Process management utilities for spawning and controlling subprocesses.                      |
| `rix-util`       | `rix::util`       | General-purpose helper functions for C++ projects.                                           |
| `rix-async`      | `rix::async`      | Coroutines, futures, and asynchronous primitives.                                            |
| `rix-thread`     | `rix::thread`     | Thread and synchronization utilities: thread creation, mutex, lock, and condition variables. |
| `rix-iterator`   | `rix::iterator`   | Iterator utilities and abstractions: range-based helpers, iterator traits, and adapters.     |
| `rix-assert`     | `rix::assert`     | Assertion utilities for runtime checks and debug validation.                                 |

---

## Installation

`rix` modules are **header-only** and can be included individually or via the umbrella `rix` CMake project.

Example using CMake:

```cmake
# Add rix umbrella as a subdirectory
add_subdirectory(path/to/rix)
target_link_libraries(my_project PRIVATE rix::rix)
```

Include modules in your C++ code:

```cpp
#include <rix/io/file.h>
#include <rix/json/json.h>
#include <rix/net/tcp.h>
#include <rix/chrono/timer.h>
```

---

## Usage Example

```cpp
#include <rix/io/file.h>
#include <rix/json/json.h>
#include <rix/net/tcp.h>
#include <rix/chrono/timer.h>

int main() {
    rix::io::File f("test.txt", rix::io::Mode::Write);
    f.write("Hello Rix!\n");

    rix::json::Json j;
    j["ok"] = true;

    rix::net::TcpClient sock("example.com", 8080);
    sock.write("PING\n");

    rix::chrono::Timer t;
    t.sleep_ms(1000);
}
```

---

## Contributing

Each module is maintained in a separate repository. Contributions should be submitted as pull requests to the corresponding module repository.

---

## License

MIT License
