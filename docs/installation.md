## Installation

### Prerequisites

Vix.cpp is a **native C++ runtime** and requires a modern toolchain.

#### All platforms
- **CMake ≥ 3.20**
- **C++20 compiler**
  - GCC ≥ 11
  - Clang ≥ 14
  - MSVC ≥ 19.34 (Visual Studio 2022)
- **Git** (with submodules)

#### Linux
- `pkg-config`
- `ninja` (recommended)
- system development packages:
  - Boost
  - OpenSSL
  - SQLite
  - zlib / brotli (optional)

**Example (Ubuntu):**
```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  libboost-all-dev libssl-dev libsqlite3-dev
```

#### macOS
- Xcode Command Line Tools
- Homebrew

```bash
brew install cmake ninja pkg-config boost openssl@3
```

#### Windows
- **Visual Studio 2022** (Desktop development with C++)
- **Git**
- **PowerShell**
- **vcpkg** (handled automatically by the install script)

---

### Install Vix.cpp (recommended)

Vix provides platform-specific install scripts that:
- fetch dependencies
- configure the build
- produce the `vix` binary

#### Linux / macOS
```bash
./install.sh
```

You may need:
```bash
chmod +x install.sh
```

#### Windows (PowerShell)
```powershell
.\install.ps1
```

> On Windows, dependencies such as **Boost** and **SQLite** are installed automatically via **vcpkg**.

---

### Verify installation

After installation, verify that `vix` is available:

```bash
vix --version
```

or on Windows:
```powershell
vix.exe --version
```

You should see the current release version printed.

---

### Script mode (no project setup)

Once installed, you can run C++ files directly:

```bash
vix run main.cpp
vix dev main.cpp
```

This compiles, links, and runs your code with the Vix runtime automatically.

---

### Manual build (advanced)

If you prefer full control, see:
- **Build & Installation**

