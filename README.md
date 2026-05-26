<table>
  <tr>
    <td valign="top" width="70%">

<h1>Vix.cpp</h1>

<p>
  <a href="https://x.com/vix_cpp">
    <img src="https://img.shields.io/badge/X-Follow-black?logo=x" />
  </a>
  <a href="https://www.youtube.com/@vixcpp">
    <img src="https://img.shields.io/badge/YouTube-Subscribe-red?logo=youtube" />
  </a>
</p>

<h3>Build real applications with modern C++.</h3>

<p>
  Vix.cpp is a modern C++ runtime for building fast, reliable, production-ready applications.
</p>

<p>
  <a href="https://vixcpp.com"><b>Website</b></a> ·
  <a href="https://docs.vixcpp.com"><b>Docs</b></a> ·
  <a href="https://registry.vixcpp.com"><b>Registry</b></a> ·
  <a href="https://blog.vixcpp.com"><b>Engineering notes</b></a>
</p>

</td>

<td valign="middle" width="13%" align="right">

<img
  src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1778607554/vix_logo_ms5lne.png"
  width="150"
  style="border-radius:50%; object-fit:cover;"
/>

</td>
  </tr>
</table>

Vix.cpp removes friction from C++ application development.
It gives C++ a modern application workflow while keeping native performance, explicit control, and production-oriented architecture.

Vix is not just a web framework.
It is a runtime foundation for backend services, AI agents, games, P2P systems, local-first applications, fast builds, templates, and production-ready C++ projects.

## Install

Shell, Linux and macOS:

```bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

PowerShell, Windows:

```powershell
irm https://vixcpp.com/install.ps1 | iex
```

More installation options:

https://vixcpp.com/install

## Quick start

Create `server.cpp`:

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
    vix::App app;

    app.get("/", [](Request &req, Response &res) {
        res.send("Hello from Vix.cpp");
    });

    app.run(8080);

    return 0;
}
```

Run it:

```bash
vix run server.cpp
```

Open:

```text
http://localhost:8080
```

## Why Vix.cpp?

C++ is powerful, but building real applications often means rebuilding the same foundation again and again.

Vix gives you that foundation:

- run C++ files with `vix run`
- create projects with `vix new`
- build fast with `vix build`
- build backend services
- build WebSocket applications
- build P2P systems
- build AI agents
- build game-oriented projects
- use threadpool and async runtime modules
- use KV, cache, database, ORM, middleware, crypto, validation, and template modules
- generate production-ready backend projects
- integrate backend projects with frontend apps such as Vue.js

## Runtime modules

```text
agent        async        cache        cli          conversion
core         crypto       db           env          error
fs           game         io           json         kv
log          middleware   net          orm          os
p2p          p2p_http     path         process      reply
sync         template     tests        threadpool   time
utils        validation   webrpc       websocket
```

Vix.cpp is designed as an application runtime layer, not only as an HTTP server.

## Built with Vix.cpp

### Kordex

A JavaScript and TypeScript runtime layer built on Vix and Softadastra.

https://github.com/softadastra/kordex

### Softadastra

A local-first and offline-first runtime foundation for reliable applications.

https://github.com/softadastra/softadastra

### PulseGrid

Real-time service monitoring built with Vix.cpp.

https://github.com/GaspardKirira/PulseGrid

### Vix Game

Game-oriented project built on the Vix.cpp runtime foundation.

https://github.com/vixcpp/vix-game

## Links

- Documentation: https://docs.vixcpp.com
- Registry: https://registry.vixcpp.com
- Engineering notes: https://blog.vixcpp.com
- Website: https://vixcpp.com

## Contributing

Contributions are welcome.

Read the contribution guide:

https://docs.vixcpp.com/contributing

## License

MIT License.
