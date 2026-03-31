# Install

This page covers:

-   Installing the Vix CLI
-   Platform prerequisites
-   Verifying your installation

------------------------------------------------------------------------

## Install Vix CLI

### Linux / macOS

``` bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

After installation, ensure `~/.local/bin` is in your PATH:

``` bash
echo $PATH
```

Verify:

``` bash
which vix
vix --version
```

------------------------------------------------------------------------

### Windows (PowerShell)

``` powershell
irm https://vixcpp.com/install.ps1 | iex
```

Verify:

``` powershell
Get-Command vix
vix --version
```

------------------------------------------------------------------------

## Build prerequisites

Vix projects are modern C++ projects. You need:

-   A C++20 compiler
-   CMake 3.20+
-   Ninja (recommended)

------------------------------------------------------------------------

### Ubuntu example

``` bash
sudo apt update
sudo apt install -y   build-essential cmake ninja-build pkg-config   libboost-all-dev libssl-dev libsqlite3-dev
```

------------------------------------------------------------------------

### macOS (Homebrew)

``` bash
brew install cmake ninja pkg-config boost openssl@3
```

------------------------------------------------------------------------

### Windows

Use:

-   Visual Studio Build Tools (MSVC)\
    or
-   clang-cl

Install dependencies using vcpkg if required by your project.

------------------------------------------------------------------------

## Troubleshooting

### "vix not found"

-   Restart your terminal
-   Ensure `~/.local/bin` is in PATH
-   Run `which vix` (Linux/macOS)
-   Run `Get-Command vix` (Windows)

------------------------------------------------------------------------

### Download failures

If installation fails:

-   Check your internet connection
-   Verify DNS resolution
-   Ensure GitHub releases are accessible from your network

