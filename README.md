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

<h3>Remove the friction from C++.</h3>

<p>
  A modern runtime for building C++ applications.
</p>

<p>
  <a href="https://vixcpp.com"><b>Website</b></a> ·
  <a href="https://vixcpp.com/docs"><b>Docs</b></a> ·
  <a href="https://vixcpp.com/install"><b>Install</b></a> ·
  <a href="https://github.com/vixcpp/vix/releases"><b>Download</b></a>
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

## Install

## <a href="https://vixcpp.com/install">Shell (Linux, macOS)</a>

```bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

## <a href="https://vixcpp.com/install">PowerShell (Windows)</a>

```powershell
irm https://vixcpp.com/install.ps1 | iex
```

## Run C++ instantly

```cpp
#include <iostream>

int main(){
  std::cout << "Hello, world!" << std::endl;
}
```

```bash
vix run main.cpp
```

Done.

## Build a server

```cpp
#include <vix.hpp>

using namespace vix;

int main(){
  App app;

  app.get("/", [](Request&, Response& res){
    res.send("Hello, world!");
  });

  app.run(8080);
}
```

```bash
vix run server.cpp
```

→ http://localhost:8080

---

## Install a framework in 1 command

```bash
vix install -g cnerium/app
```

```cpp
#include <cnerium/app/app.hpp>
using namespace cnerium::app;

int main(){
  App app;

  app.get("/", [](AppContext &ctx){
    ctx.text("Hello from Cnerium");
  });

  app.listen("127.0.0.1", 8080);
}
```

```bash
vix run main.cpp
```

## WebSocket

```cpp
#include <memory>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/websocket.hpp>

int main(){
  auto exec = std::make_shared<vix::executor::RuntimeExecutor>();

  vix::websocket::App app{"config/config.json", exec};
  auto &ws = app.server();

  ws.on_typed_message([](auto &,
                         const std::string &type,
                         const vix::json::kvs &payload)
  {
    if (type == "chat.message")
      return payload;
  });

  app.run_blocking();
}
```

## What Vix.cpp gives you

- Run a single `.cpp` file instantly
- No CMake required for simple apps
- Native C++ performance
- HTTP, WebSocket, P2P ready
- Offline-first architecture support
- Deterministic execution


## Why Vix exists

C++ is powerful.

But:
- too much setup
- too much friction
- too slow to start

Vix removes that.

## Performance

Stable under sustained load.

| Metric        | Value          |
|--------------|----------------|
| Requests/sec | ~66k – 68k     |
| Avg Latency  | ~13–20 ms      |
| P99 Latency  | ~17–50 ms      |

## Learn more

- Docs: https://vixcpp.com/docs
- Registry: https://vixcpp.com/registry
- Examples: https://vixcpp.com/docs/examples

## Contributing

Contributions are welcome.

### Focus areas

- performance
- reliability
- networking
- offline-first systems

MIT License
