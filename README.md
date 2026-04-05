<table>
  <tr>
    <td valign="top" width="65%">

<h1>Vix.cpp</h1>

<p>
  <a href="https://x.com/vix_cpp">
    <img src="https://img.shields.io/badge/X-Follow-black?logo=x" />
  </a>
  <a href="https://www.youtube.com/@vixcpp">
    <img src="https://img.shields.io/badge/YouTube-Subscribe-red?logo=youtube" />
  </a>
</p>

<p>
  <b>A modern C++ runtime for real-world systems.</b>
</p>

<p>
  Build HTTP, WebSocket, and peer-to-peer applications with
  <b>predictable performance</b> and <b>offline-first reliability</b>.
</p>

<p>
  🌍 <a href="https://vixcpp.com">Website</a> ·
  📘 <a href="https://vixcpp.com/docs">Docs</a> ·
  ⬇️ <a href="https://github.com/vixcpp/vix/releases">Download</a>
</p>

</td>

<td valign="middle" width="25%" align="right">

<img
  src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1774966011/vix_logo_yqjne1.png"
  width="260"
  style="border-radius:50%; object-fit:cover;"
/>

</td>
  </tr>
</table>

---

## Contents

- [Install](#install)
- [Build from source](#build-from-source)
- [Your first Vix.cpp program](#your-first-vixcpp-program)
- [Script mode](#script-mode-no-project-setup)
- [Shell completion](#shell-completion)
- [Why Vix.cpp](#why-vixcpp)
- [Performance](#performance)
- [Core principles](#core-principles)
- [Learn more](#learn-more)
- [Contributing](#contributing)

---

## Install

#### Linux

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config git curl unzip zip \
  libssl-dev libsqlite3-dev zlib1g-dev \
  nlohmann-json3-dev \
  libspdlog-dev libfmt-dev
```

## macOS Dependencies (example)

```bash
brew install cmake ninja pkg-config openssl@3 spdlog fmt nlohmann-json
```

## <a href="https://vixcpp.com/install">Shell (Linux, macOS)</a>

```bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

## <a href="https://vixcpp.com/install">PowerShell (Windows)</a>

```powershell
irm https://vixcpp.com/install.ps1 | iex
```

Verify installation:

```bash
vix --version
```

## Build from source
```bash
git clone --recurse-submodules https://github.com/vixcpp/vix.git
cd vix

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Install for current user (recommended)
cmake --install build --prefix "$HOME/.local"

# Ensure PATH contains ~/.local/bin then restart your terminal
vix --version
vix doctor
```

## Your first Vix.cpp program

Create a file called `server.cpp`:

```cpp
#include <vix.hpp>
using namespace vix;

int main() {
  App app;

  app.get("/", [](Request&, Response& res) {
    res.send("Hello, world!");
  });

  app.run(8080);
}
```

Run it:

```bash
vix run server.cpp
```

Open http://localhost:8080
That’s it.

---

## Script mode (no project setup)

Run C++ like a script:

```bash
vix run main.cpp
```

## Shell completion

Enable tab completion for Vix commands.

```bash
source <(vix completion bash)
```

Make it permanent:

```bash
vix completion bash > ~/.vix-completion.bash
echo 'source ~/.vix-completion.bash' >> ~/.bashrc
```

Learn more: https://vixcpp.com/docs/modules/cli/completion

## Why Vix.cpp

Most systems assume perfect conditions.
Vix is built for when things are not.

- predictable under load
- no GC pauses
- offline-first by design
- deterministic execution
- minimal setup

---

## Performance

Stable under sustained load.

| Metric        | Value          |
|--------------|----------------|
| Requests/sec | ~66k – 68k     |
| Avg Latency  | ~13–20 ms      |
| P99 Latency  | ~17–50 ms      |

---

## Core principles

- Local-first execution
- Network is optional
- Deterministic behavior
- Failure-tolerant
- Built for unreliable environments

---

## Learn more

- Docs: https://vixcpp.com/docs
- Registry: https://vixcpp.com/registry
- Examples: https://vixcpp.com/docs/examples

---

## Contributing

Contributions are welcome.

### Focus areas

- performance
- reliability
- networking
- offline-first systems

---

MIT License
