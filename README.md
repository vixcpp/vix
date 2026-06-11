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
  Vix.cpp is a modern C++ runtime and developer toolkit for building fast, reliable, production-ready applications.
</p>

<p>
  <a href="https://vixcpp.com"><b>Website</b></a> ·
  <a href="https://docs.vixcpp.com"><b>Docs</b></a> ·
  <a href="https://rix.vixcpp.com"><b>Rix</b></a> ·
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

Vix is not only a web framework. It is a runtime foundation for backend services, WebSocket applications, AI agents, games, P2P systems, local-first applications, fast builds, templates, package-based projects, and production-ready C++ services.

## Install

Linux and macOS:

```bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

Windows PowerShell:

```powershell
irm https://vixcpp.com/install.ps1 | iex
```

More installation options:

```text
https://vixcpp.com/install
```

## Quick start

Create `server.cpp`:

```cpp
#include <vix.hpp>

int main()
{
    vix::App app;

    app.get("/", [](vix::Request &req, vix::Response &res) {
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

## Why Vix.cpp ?

C++ is powerful, but building real applications often means rebuilding the same foundation again and again.

Vix gives you that foundation:

- run C++ files with `vix run`
- create structured projects with `vix new`
- build projects with `vix build`
- run tests with `vix tests`
- manage packages through the Vix registry
- build HTTP backend services
- build WebSocket applications
- build P2P systems
- build AI agent projects
- build game-oriented projects
- use async and threadpool modules
- use KV, cache, database, ORM, middleware, crypto, validation, JSON, template, and process modules
- generate production-ready backend projects
- integrate backend projects with frontend applications such as Vue.js

Vix is designed to make C++ feel usable for real application development without hiding what makes C++ powerful.

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

## Vix Reply

Vix Reply is the interactive REPL engine for Vix.

It powers the interactive `vix` and `vix repl` experience, with support for expressions, variables, JSON values, runtime helpers, and real C++ snippets powered by the normal `vix run` pipeline.

Start the REPL:

```bash
vix
```

Or explicitly:

```bash
vix repl
```

Run real C++ from the REPL:

```text
>>> :cpp
C++ mode. Type :run to execute or :cancel to exit.
cpp> #include <vix/print.hpp>
...   int main() {
...     vix::print("Hello from C++");
...   }
Hello from C++
```

Vix Reply is not a fake C++ interpreter. C++ snippets are written to a temporary `.cpp` file and executed through `vix run`, so the code goes through the real compiler, Vix diagnostics, and the normal runtime behavior.

## Registry and packages

Vix includes registry integration for package-based C++ projects.

```bash
vix add rix/csv
vix install
vix build
```

The registry makes it possible to build a larger ecosystem around Vix without forcing every library into the core runtime.

## Rix

Rix is the official userland library layer for Vix.cpp.

It provides optional public libraries that live in the registry and can evolve independently from Vix Core.

Vix stays focused on the runtime, CLI, build workflow, registry integration, and existing core modules.

Rix provides reusable libraries for application developers.

Examples:

```bash
vix add rix/csv
vix add rix/debug
vix add rix/auth
vix add rix/pdf
```

Rix packages can be used independently, or through a unified facade package when a project wants one clean entry point.

```cpp
rix.csv.parse(...)
rix.debug.print(...)
rix.auth.register_user(...)
rix.pdf.document()
```

Rix does not replace Vix.

It grows around Vix as the public library layer for real applications.

## Cnerium

Cnerium is a reliability-first backend layer for Vix.

It attaches to an existing Vix backend and adds durable route behavior for critical write operations.

Cnerium is designed for routes that must stay correct under retries, timeouts, lost responses, process restarts, and unstable networks.

```cpp
#include <vix.hpp>
#include <cnerium/cnerium.hpp>

int main()
{
    vix::App app;

    auto c = cnerium::attach(app);

    c.durable_post(
        "/orders",
        "orders.create",
        [](cnerium::DurableRequest &request) {
            return cnerium::created({
                {"ok", true}
            });
        });

    c.start();
    app.run();
}
```

Cnerium does not replace Vix. Vix remains the backend runtime, HTTP server, router, WebSocket transport, build workflow, and application foundation.

Cnerium attaches to Vix and protects selected write operations.

## Built with Vix.cpp

| Project                                                             | Description                                                                                                                      |
| ------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| [Rix](https://github.com/rixcpp/rix)                                | Official userland library layer for Vix.cpp, distributed through the registry.                                                   |
| [Vix Pico](https://github.com/vixcpp/pico)                          | Production-style backend used to validate Vix modules, deployment, auth, PDF generation, SQLite, KV, jobs, and WebSocket routes. |
| [Vix Game](https://github.com/vixcpp/vix-game)                      | Game-oriented project built on the Vix.cpp runtime foundation.                                                                   |
| [Softadastra Runner](https://github.com/softadastra/runner)         | Small command runner built with Vix.cpp and `vix::process`.                                                                      |
| [Softadastra Kordex](https://github.com/softadastra/kordex)         | JavaScript and TypeScript runtime layer built on Vix and Softadastra.                                                            |
| [Softadastra Engine](https://github.com/softadastra/softadastra)    | Local-first and offline-first runtime foundation for reliable applications.                                                      |
| [Softadastra Cnerium](https://github.com/softadastra/cnerium)       | Reliability-first application framework built on Vix and the Softadastra SDK.                                                    |
| [Softadastra Cloud](https://github.com/softadastra/cloud)           | Reliability testing control plane for reports, scores, and dashboard results.                                                    |
| [Softadastra Converdict](https://github.com/softadastra/converdict) | Reliability verification platform for distributed systems.                                                                       |
| [Softadastra PulseGrid](https://github.com/softadastra/PulseGrid)   | Real-time service monitoring built with Vix.cpp.                                                                                 |

## Vix Pico

Vix Pico is a production-style backend built with Vix.cpp.

It validates Vix in real backend conditions:

- HTTP routes
- WebSocket runtime
- SQLite persistence
- KV storage
- threadpool jobs
- runtime events
- production service deployment
- Rix Auth diagnostics
- Rix PDF generation

Pico is used to prove that Vix can build and deploy real backend applications, not only small examples.

## Softadastra Runner

Softadastra Runner is a simple command runner built with Vix.cpp.

It shows how to structure a small console application around:

- application orchestration
- CLI input and output
- command parsing
- service logic
- process execution through `vix::process`

Run it with:

```bash
vix build
vix run
```

Runner is intentionally small, readable, and useful as a clean example for building console tools with Vix.cpp.

## Project workflow

Create a new project:

```bash
vix new hello --app
cd hello
```

Build it:

```bash
vix build
```

Run it:

```bash
vix run
```

Run tests:

```bash
vix tests
```

Add a package:

```bash
vix add rix/csv
vix install
```

## Production workflow

Vix provides a production workflow for backend applications.

A basic deployment can be handled with:

```bash
vix deploy
```

`vix deploy` can build the application, restart the service, check service status, run health checks, validate the proxy, and show logs when something fails.

For production inspection and operations, Vix also provides:

```bash
vix doctor production
vix service status
vix logs
vix health
vix proxy nginx check
```

A typical production flow looks like this:

```bash
vix build
vix service install
vix proxy nginx init
vix deploy
vix doctor production
```

The idea is to avoid custom deployment scripts for normal Vix backend applications.

Vix should make the production state visible: service, ports, proxy, TLS, health checks, logs, and runtime status.

## Links

- Website: https://vixcpp.com
- Documentation: https://docs.vixcpp.com
- Rix documentation: https://rix.vixcpp.com
- Registry: https://registry.vixcpp.com
- Engineering notes: https://blog.vixcpp.com
- X: https://x.com/vix_cpp
- YouTube: https://www.youtube.com/@vixcpp

## Contributing

Contributions are welcome.

Read the contribution guide:

```text
https://docs.vixcpp.com/contributing
```

You can contribute by improving the runtime, writing examples, testing Vix on real projects, improving documentation, or publishing packages for the Vix registry.

## License

MIT License.
