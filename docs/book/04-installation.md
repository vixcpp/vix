# Installation

Now you will install Vix.

## Install

**Linux and macOS:**

```bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

**Windows PowerShell:**

```powershell
irm https://vixcpp.com/install.ps1 | iex
```

## SDK mode vs CLI-only mode

| Mode                 | What it installs                 | Use when                                  |
|----------------------|----------------------------------|-------------------------------------------|
| SDK mode, default    | CLI, headers, and libraries.     | You want to compile Vix applications.     |
| CLI-only mode        | The `vix` binary only.           | You only need the CLI without compiling.  |

For this book, install the full SDK (default). CLI-only mode cannot compile projects that use `#include <vix.hpp>`.

```bash
# CLI-only (not recommended for this book)
VIX_INSTALL_KIND=cli curl -fsSL https://vixcpp.com/install.sh | bash
```

## Verify the CLI

```bash
vix --version
```

Expected output shape:

```txt
Vix.cpp CLI
version : 2.5.2
author  : Gaspard Kirira
source  : https://github.com/vixcpp/vix
```

## Verify the SDK headers

```bash
find ~/.local/include -name vix.hpp 2>/dev/null
# Expected: ~/.local/include/vix.hpp
```

## PATH check

If `vix: command not found`:

```bash
# Bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# Zsh
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

## Build prerequisites

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  libssl-dev libsqlite3-dev zlib1g-dev libbrotli-dev \
  nlohmann-json3-dev libspdlog-dev libfmt-dev
```

### macOS (Homebrew)

```bash
brew install cmake ninja pkg-config openssl@3 spdlog fmt nlohmann-json brotli
```

### Windows

Use Visual Studio Build Tools with MSVC or clang-cl. For extra dependencies, use vcpkg.

## Create your first project

```bash
vix new app
cd app
vix dev
```

## Run a single file

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.json({"message", "Hello from Vix"});
  });

  app.run(8080);
  return 0;
}
```

```bash
vix run main.cpp
curl -i http://127.0.0.1:8080/
```

## Common issues

### `vix: command not found`

Fix `PATH` — add `~/.local/bin` and restart your terminal.

### `#include <vix.hpp>` not found

```bash
find ~/.local/include -name vix.hpp 2>/dev/null
```

If nothing appears, reinstall the full SDK. Do not use CLI-only mode.

### Compiler or CMake missing

```bash
c++ --version
cmake --version
ninja --version
```

Install missing tools with `apt install build-essential cmake ninja-build`.

## Build from source

```bash
git clone --recursive https://github.com/vixcpp/vix.git
cd vix

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DVIX_ENABLE_INSTALL=ON

cmake --build build -j
cmake --install build
```

## Upgrade later

```bash
vix upgrade     # upgrade the CLI
vix doctor      # check environment
vix info        # inspect paths and caches
```

## What you should remember

For this book, install the full SDK:

```bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

Then verify:

```bash
vix --version
find ~/.local/include -name vix.hpp 2>/dev/null
```

CLI-only is for the binary. SDK mode is for building Vix applications.

## Next chapter

[Next: Run your first file](/book/05-run-your-first-file)
