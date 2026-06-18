# Installation

This page shows how to install Vix.cpp and verify that it works on your machine.
For Getting Started and real C++ application development, install the full SDK.
Starting with **Vix.cpp v2.6.0**, the recommended installation is the full SDK installation.

The full SDK includes:

- the `vix` CLI
- the main `vix.hpp` header
- Vix module headers
- Vix static libraries
- CMake package files
- the `vix::vix` target for CMake projects

This means you can install Vix once, then build real Vix applications without manually copying headers, linking modules, or rebuilding Vix yourself.

## Recommended install

Linux and macOS:

```bash
curl -fsSL https://vixcpp.com/install.sh | sh
```

Windows PowerShell:

```powershell
irm https://vixcpp.com/install.ps1 | iex
```

After installation, restart your terminal.

Then verify the CLI:

```bash
vix --version
```

Expected output shape:

```txt
Vix.cpp CLI
version : v2.6.1
author  : Gaspard Kirira
source  : https://github.com/vixcpp/vix
```

The exact version may be newer depending on the latest release.

## What the full SDK installs

The full SDK installs the command-line tool and the development files needed by C++ projects.

It installs files like:

```txt
~/.local/bin/vix
~/.local/include/vix.hpp
~/.local/include/vix/...
~/.local/lib/libvix_*.a
~/.local/lib/cmake/Vix/VixConfig.cmake
~/.local/lib/cmake/Vix/VixTargets.cmake
```

For CMake projects, the expected usage is:

```cmake
find_package(Vix CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE vix::vix)
```

The `vix::vix` target is the main SDK target. It is designed to provide the complete Vix development foundation.

## Verify the CLI

Check that the `vix` command is available:

```bash
vix --version
```

If your terminal says:

```txt
vix: command not found
```

your shell cannot find the Vix binary.

Add `~/.local/bin` to your `PATH`.

### Bash

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### Zsh

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

Then check again:

```bash
vix --version
```

## Verify the SDK

Check that the main SDK header exists:

```bash
find ~/.local/include -name vix.hpp 2>/dev/null
```

Expected output shape:

```txt
/home/your-user/.local/include/vix.hpp
```

Check that the Vix CMake package exists:

```bash
find ~/.local/lib/cmake -name VixConfig.cmake 2>/dev/null
```

Expected output shape:

```txt
/home/your-user/.local/lib/cmake/Vix/VixConfig.cmake
```

Check that Vix static libraries are installed:

```bash
find ~/.local/lib -name "libvix_*.a" 2>/dev/null
```

For example, if you use WebSocket features, this file should exist:

```bash
find ~/.local/lib -name "libvix_websocket.a" 2>/dev/null
```

Expected output shape:

```txt
/home/your-user/.local/lib/libvix_websocket.a
```

If these files exist, the SDK is installed.

## Verify with a simple C++ file

Create a temporary folder:

```bash
mkdir -p ~/tmp/vix-install-test
cd ~/tmp/vix-install-test
```

Create `main.cpp`:

```bash
cat > main.cpp <<'CPP'
#include <vix.hpp>

int main()
{
  vix::print("Hello from Vix.cpp");
  return 0;
}
CPP
```

Run it:

```bash
vix run main.cpp
```

Expected output:

```txt
Hello from Vix.cpp
```

If this works, your CLI and SDK are ready.

## Verify with a CMake project

Create a temporary CMake project:

```bash
mkdir -p ~/tmp/vix-cmake-test
cd ~/tmp/vix-cmake-test
```

Create `CMakeLists.txt`:

```bash
cat > CMakeLists.txt <<'CMAKE'
cmake_minimum_required(VERSION 3.20)
project(vix_cmake_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vix CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE vix::vix)
CMAKE
```

Create `main.cpp`:

```bash
cat > main.cpp <<'CPP'
#include <vix.hpp>

int main()
{
  vix::print("Hello from Vix CMake");
  return 0;
}
CPP
```

Configure and build:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local"
cmake --build build
./build/app
```

Expected output:

```txt
Hello from Vix CMake
```

## Verify with a Vix project

Create a project:

```bash
vix new api
cd api
```

Build it:

```bash
vix build
```

Run it:

```bash
vix run
```

If the application starts, your installation is correct.

## Updating Vix

There are two update paths.

### Update the CLI

`vix upgrade` updates the installed Vix CLI binary:

```bash
vix upgrade
```

This updates the command-line tool only.

It does not reinstall the full SDK.

That means `vix upgrade` does not replace:

- installed headers
- static libraries
- CMake package files
- module libraries such as `libvix_websocket.a`

Use `vix upgrade` when you only need the latest `vix` command.

### Update the full SDK

If you build C++ applications with Vix, update the full SDK with:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_INSTALL_KIND=sdk sh
```

To install or update a specific SDK version:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_VERSION=v2.6.1 VIX_INSTALL_KIND=sdk sh
```

Use the SDK update command when your project depends on:

```cpp
#include <vix.hpp>
```

or:

```cmake
find_package(Vix CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE vix::vix)
```

## CLI update vs SDK update

| Command                                                                                  | Updates                                          | Use when                          |
| ---------------------------------------------------------------------------------------- | ------------------------------------------------ | --------------------------------- |
| `vix upgrade`                                                                            | CLI binary only                                  | You want the latest `vix` command |
| `curl -fsSL https://vixcpp.com/install.sh \| VIX_INSTALL_KIND=sdk sh`                    | CLI, headers, libraries, and CMake package files | You build Vix C++ applications    |
| `curl -fsSL https://vixcpp.com/install.sh \| VIX_VERSION=v2.6.1 VIX_INSTALL_KIND=sdk sh` | Full SDK for a specific version                  | You need a known SDK version      |

For application development, use the SDK installation or SDK update command.

## SDK mode vs CLI-only mode

Vix has two installation modes.

| Mode          | What it installs                                 | Use when                            |
| ------------- | ------------------------------------------------ | ----------------------------------- |
| SDK mode      | CLI, headers, libraries, and CMake package files | You want to build Vix applications  |
| CLI-only mode | Only the `vix` binary                            | You only need the command-line tool |

For Getting Started, use **SDK mode**.

Do not use CLI-only mode if you want to compile code that includes:

```cpp
#include <vix.hpp>
```

Do not use CLI-only mode if you want to build projects that use:

```cmake
find_package(Vix CONFIG REQUIRED)
```

## CLI-only install

CLI-only mode installs only the command-line tool.

Linux and macOS:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_INSTALL_KIND=cli sh
```

This is not recommended for Getting Started.

The next pages build real Vix applications, so you need the full SDK.

## Install a specific version

By default, the installer uses the latest release.

To install a specific SDK version on Linux or macOS:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_VERSION=v2.6.1 VIX_INSTALL_KIND=sdk sh
```

To install only the CLI for a specific version:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_VERSION=v2.6.1 VIX_INSTALL_KIND=cli sh
```

On Windows PowerShell:

```powershell
$env:VIX_VERSION="v2.6.1"
$env:VIX_INSTALL_KIND="sdk"
irm https://vixcpp.com/install.ps1 | iex
```

For CLI-only mode on Windows PowerShell:

```powershell
$env:VIX_VERSION="v2.6.1"
$env:VIX_INSTALL_KIND="cli"
irm https://vixcpp.com/install.ps1 | iex
```

## Install build prerequisites

Vix installs the SDK, but it still uses the normal C++ toolchain underneath.

You need a compiler, CMake, Ninja, and the system libraries used by the modules you want to build.

### Ubuntu or Debian

Recommended base setup:

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  ca-certificates git curl unzip zip tar \
  libssl-dev libsqlite3-dev zlib1g-dev libbrotli-dev \
  nlohmann-json3-dev libspdlog-dev libfmt-dev
```

If you want to use database modules with MySQL:

```bash
sudo apt install -y libmysqlcppconn-dev
```

If you want to use the Vix game module with SDL/OpenGL:

```bash
sudo apt install -y \
  libsdl2-dev libsdl2-image-dev libgl1-mesa-dev
```

If you want to use the Vix AI agent with a local model, install Ollama:

```bash
curl -fsSL https://ollama.com/install.sh | sh
```

Then pull a small model for low-power machines:

```bash
ollama pull llama3.2:1b
```

Or pull a small coding-oriented model:

```bash
ollama pull qwen2.5-coder:1.5b
```

For most laptops, start with:

```bash
ollama pull llama3.2:1b
```

It is smaller and easier to run than larger models.

### macOS

With Homebrew:

```bash
brew install cmake ninja pkg-config openssl@3 spdlog fmt nlohmann-json brotli
```

For the game module:

```bash
brew install sdl2 sdl2_image
```

For the AI agent with a local model:

```bash
brew install ollama
```

Start Ollama:

```bash
ollama serve
```

Then pull a small model:

```bash
ollama pull llama3.2:1b
```

Or a small coding-oriented model:

```bash
ollama pull qwen2.5-coder:1.5b
```

### Windows

Install one C++ toolchain:

- Visual Studio Build Tools with MSVC
- Visual Studio with the Desktop development with C++ workload
- clang-cl

Install CMake and Ninja.

For extra dependencies, use `vcpkg`.

If you want to use the AI agent with a local model, install Ollama for Windows from the official Ollama website, then run:

```powershell
ollama pull llama3.2:1b
```

Or:

```powershell
ollama pull qwen2.5-coder:1.5b
```

## Module-specific dependencies

The full Vix SDK includes the Vix modules, but some modules rely on system libraries.

| Module area      | System dependency        | When you need it                               |
| ---------------- | ------------------------ | ---------------------------------------------- |
| Core build       | compiler, CMake, Ninja   | Always                                         |
| Crypto / TLS     | OpenSSL                  | When using crypto, TLS, HTTPS-related features |
| SQLite           | SQLite3                  | When using SQLite database support             |
| MySQL            | MySQL C++ Connector      | When using MySQL database support              |
| HTTP compression | zlib, Brotli             | When using gzip or Brotli compression          |
| Game             | SDL2, SDL2_image, OpenGL | When using the SDL/OpenGL game backend         |
| Agent            | Ollama                   | Only when running local AI models              |

Ollama is not required to install Vix.

Ollama is only needed if you want to run local AI agent features such as:

```bash
vix agent ask
vix agent analyze
vix agent scan
```

## Recommended local AI model

For low-power machines, use:

```bash
ollama pull llama3.2:1b
```

This is the best first model to recommend because it is small and easier to run.

For coding-focused tests, use:

```bash
ollama pull qwen2.5-coder:1.5b
```

Then you can test the agent:

```bash
vix agent ask "Explain this project"
```

If the model is slow on first run, that is normal. Local models often need more time on the first request.

## Check your toolchain

Run:

```bash
c++ --version
cmake --version
ninja --version
```

If one of these commands is missing, install the missing tool before continuing.

## Useful commands after installation

Check the installed version:

```bash
vix --version
```

Inspect your environment:

```bash
vix doctor
```

Show Vix paths and cache information:

```bash
vix info
```

Update the CLI:

```bash
vix upgrade
```

Update the full SDK:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_INSTALL_KIND=sdk sh
```

These commands are useful when you want to understand what Vix installed, which paths are used, and whether your environment is ready.

## Common installation problems

### `vix: command not found`

Your shell cannot find the Vix binary.

Fix for Bash:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Then run:

```bash
vix --version
```

Fix for Zsh:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

Then run:

```bash
vix --version
```

### `#include <vix.hpp>` not found

The full SDK is not installed, or your project is not using the SDK path.

Check:

```bash
find ~/.local/include -name vix.hpp 2>/dev/null
```

If nothing appears, reinstall the full SDK:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_INSTALL_KIND=sdk sh
```

Then restart your terminal and check again:

```bash
find ~/.local/include -name vix.hpp 2>/dev/null
```

### `find_package(Vix CONFIG REQUIRED)` fails

Check that the CMake package exists:

```bash
find ~/.local/lib/cmake -name VixConfig.cmake 2>/dev/null
```

If nothing appears, reinstall the full SDK:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_INSTALL_KIND=sdk sh
```

If it exists but CMake cannot find it, pass the SDK prefix manually:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local"
```

Then build:

```bash
cmake --build build
```

### `vix upgrade` worked, but my project still fails to link

`vix upgrade` updates the CLI binary only.

It does not reinstall the SDK headers, libraries, or CMake package files.

If your project fails with an error like:

```txt
undefined symbol: vix::websocket::LowLevelServer::run(...)
```

or:

```txt
undefined symbol: vix::websocket::Session::shutdown_now(...)
```

then your SDK libraries are missing or outdated.

Check:

```bash
find ~/.local/lib -name "libvix_websocket.a" 2>/dev/null
```

If nothing appears, reinstall the full SDK:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_INSTALL_KIND=sdk sh
```

For a specific version:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_VERSION=v2.6.1 VIX_INSTALL_KIND=sdk sh
```

Then rebuild your project:

```bash
rm -rf build build-ninja
vix build
```

### CMake or Ninja is missing

Check:

```bash
cmake --version
ninja --version
```

On Ubuntu or Debian:

```bash
sudo apt install -y cmake ninja-build
```

### The project builds but cannot find system libraries

Install the common development packages.

Ubuntu or Debian:

```bash
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  libssl-dev libsqlite3-dev zlib1g-dev libbrotli-dev \
  nlohmann-json3-dev libspdlog-dev libfmt-dev
```

Then rebuild your project:

```bash
vix build
```

### The game module cannot find SDL2 or OpenGL

Install the game dependencies.

Ubuntu or Debian:

```bash
sudo apt install -y \
  libsdl2-dev libsdl2-image-dev libgl1-mesa-dev
```

macOS:

```bash
brew install sdl2 sdl2_image
```

Then rebuild:

```bash
vix build
```

### `vix agent` cannot use a local model

Make sure Ollama is installed:

```bash
ollama --version
```

Make sure a model is installed:

```bash
ollama list
```

If no model is available, pull a small one:

```bash
ollama pull llama3.2:1b
```

Then try again:

```bash
vix agent ask "Explain this project"
```

### The first AI agent request is slow

This is normal for local AI models.

The first request can be slower because the model may need to start, load into memory, or initialize its runtime.

For low-power machines, start with:

```bash
ollama pull llama3.2:1b
```

If you want a small coding-oriented model:

```bash
ollama pull qwen2.5-coder:1.5b
```

## Clean SDK reinstall

If your system has an older or incomplete SDK installation, reinstall the SDK.

Linux and macOS:

```bash
rm -f "$HOME/.local/lib/libvix_"*.a
rm -rf "$HOME/.local/lib/cmake/Vix"

curl -fsSL https://vixcpp.com/install.sh | VIX_INSTALL_KIND=sdk sh
```

For a specific version:

```bash
rm -f "$HOME/.local/lib/libvix_"*.a
rm -rf "$HOME/.local/lib/cmake/Vix"

curl -fsSL https://vixcpp.com/install.sh | VIX_VERSION=v2.6.1 VIX_INSTALL_KIND=sdk sh
```

Then verify:

```bash
vix --version
find ~/.local/include -name vix.hpp 2>/dev/null
find ~/.local/lib/cmake -name VixConfig.cmake 2>/dev/null
find ~/.local/lib -name "libvix_websocket.a" 2>/dev/null
```

## What you should remember

For real C++ development, install the full SDK:

```bash
curl -fsSL https://vixcpp.com/install.sh | sh
```

To update only the CLI:

```bash
vix upgrade
```

To update the full SDK:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_INSTALL_KIND=sdk sh
```

To install a specific SDK version:

```bash
curl -fsSL https://vixcpp.com/install.sh | VIX_VERSION=v2.6.1 VIX_INSTALL_KIND=sdk sh
```

Verify the CLI:

```bash
vix --version
```

Verify the SDK header:

```bash
find ~/.local/include -name vix.hpp 2>/dev/null
```

Verify the CMake package:

```bash
find ~/.local/lib/cmake -name VixConfig.cmake 2>/dev/null
```

Verify the WebSocket module library:

```bash
find ~/.local/lib -name "libvix_websocket.a" 2>/dev/null
```

Inspect the environment:

```bash
vix doctor
```

For Getting Started, SDK mode is the correct installation mode.

## Next step

Now set up your development environment.

Next: [Set Up Your Environment](/getting-started/setup-environment)
