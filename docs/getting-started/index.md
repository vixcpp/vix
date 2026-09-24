# Welcome to Vix.cpp

Vix.cpp is a modern C++ runtime and developer toolkit for building native applications with a clearer development workflow.

It keeps the C++ model intact: real C++ source files, real compilers, real native binaries, and compatibility with the existing build ecosystem. What Vix.cpp adds is the operational layer around the project: commands, project structure, runtime modules, diagnostics, tests, formatting, packaging, and development workflows.

Current version: **v2.6.0**

A single C++ file can be run directly:

```bash
vix run main.cpp
```

A project can be created, built, and started through the same command surface:

```bash
vix new api
cd api
vix build
vix run
```

Vix.cpp is not only an HTTP framework. It is a runtime foundation and developer toolkit for backend services, JSON APIs, WebSocket applications, CLI tools, AI agents, games, P2P systems, local-first systems, templates, fast builds, and production-oriented C++ projects.

## Offline PDF

You can download the complete Vix.cpp documentation as a PDF:

- [Download the Vix.cpp Documentation PDF](/vixcpp-documentation.pdf)
- [Download the Vix.cpp Documentation PDF in French](/vixcpp-documentation-fr.pdf)

## What Vix.cpp is for

C++ gives developers performance, control, portability, and access to a mature ecosystem. The challenge is often the workflow around the language.

A real C++ application usually needs more than source files:

- project creation
- build configuration
- dependency setup
- runtime commands
- tests
- formatting
- logs
- diagnostics
- packaging
- deployment preparation

These pieces are often assembled manually with CMake files, shell scripts, CI configuration, package tools, formatting tools, and project-specific conventions.

Vix.cpp brings the common parts of that workflow into one application-oriented toolchain.

```txt
C++ source code
  -> Vix.cpp workflow
  -> native executable or library
```

The goal is not to replace C++. The goal is to make the path from C++ code to a working application more direct.

## How Vix.cpp fits into the C++ ecosystem

Vix.cpp works above the native C++ toolchain.

It can use CMake and Ninja when needed. It can work with existing `CMakeLists.txt` projects. For simpler projects, it can use a `vix.app` manifest and generate the internal CMake project automatically.

```txt
Existing CMake project:
  CMakeLists.txt -> Vix workflow -> build and run

Simple Vix project:
  vix.app -> generated CMake -> build and run
```

This keeps Vix.cpp compatible with the C++ ecosystem while giving developers a cleaner command surface for daily work.

## What you can build

With Vix.cpp, you can build:

- backend services
- HTTP servers
- JSON APIs
- WebSocket applications
- command-line tools
- C++ libraries
- template-based web applications
- AI agent applications
- game-oriented projects
- P2P systems
- local-first and offline-first systems
- production services behind Nginx and systemd

Different project types can use different modules, but the development workflow remains familiar.

## Quick example

Create `server.cpp`:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.text("Hello from Vix.cpp");
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

```txt
http://localhost:8080/
```

This is still native C++. Vix.cpp provides the workflow that builds and runs it.

## The core workflow

Run a single C++ file:

```bash
vix run main.cpp
```

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

Start development mode:

```bash
vix dev
```

Run checks and tests:

```bash
vix check
vix tests
```

Format source files:

```bash
vix fmt
```

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

https://vixcpp.com/install

## Runtime modules

Vix.cpp is designed as an application runtime layer, not only as a web server.

It includes modules for application development, networking, storage, diagnostics, concurrency, and production-oriented workflows.

```txt
agent        async        cache        cli          conversion
core         crypto       db           env          error
fs           game         io           json         kv
log          middleware   net          orm          os
p2p          p2p_http     path         process      reply
sync         template     tests        threadpool   time
utils        validation   webrpc       websocket
```

These modules let Vix.cpp support different application shapes while keeping a common foundation.

A backend service may use HTTP, JSON, validation, middleware, logging, database access, and tests.

A CLI tool may use filesystem utilities, formatted output, argument handling, packaging, and diagnostics.

A local-first application may use storage, sync, caching, P2P, and reliability-oriented modules.

## Getting Started path

This section gives the shortest path from installation to a running Vix application.

Read it in order:

1. [What is Vix.cpp?](/getting-started/what-is-vixcpp)
2. [Installation](/getting-started/installation)
3. [Set Up Your Environment](/getting-started/setup-environment)
4. [Run Your First C++ File](/getting-started/run-your-first-file)
5. [Create Your First Project](/getting-started/create-your-first-project)
6. [Your First HTTP Server](/getting-started/first-http-server)

The goal is to understand the workflow first, then move into deeper concepts.

## Getting Started vs The Vix Book

Getting Started is practical. It focuses on the first path:

```txt
install -> verify -> run -> create project -> start server
```

The Vix Book goes deeper. It explains the mental model behind Vix.cpp, then covers application structure, routes, requests, responses, JSON APIs, middleware, validation, database access, WebSocket, async runtime, cache, sync, P2P, and production deployment.

Start with Getting Started if you want to run something quickly.

Continue with The Vix Book when you want to understand the design and build larger applications.

## What you need

You only need basic C++ knowledge to begin:

- functions
- headers
- `std::string`
- lambdas
- basic terminal usage

You do not need to be a CMake expert to start.

Vix.cpp can create a project, build it, run it, test it, format it, and give you a clean development loop.

## Built with Vix.cpp

### Kordex

A JavaScript and TypeScript runtime layer built on Vix.cpp and Softadastra.

https://github.com/softadastra/kordex

### Softadastra

A local-first and offline-first runtime foundation for reliable applications.

https://github.com/softadastra/softadastra

### PulseGrid

Real-time service monitoring built with Vix.cpp.

https://github.com/softadastra/PulseGrid

### Vix Game

A game-oriented project built on the Vix.cpp runtime foundation.

https://github.com/vixcpp/vix-game

## Links

- Website: https://vixcpp.com
- Registry: https://registry.vixcpp.com
- Engineering notes: https://blog.vixcpp.com
- GitHub: https://github.com/vixcpp/vix

## First command to remember

```bash
vix run main.cpp
```

This is the fastest way to try Vix.cpp with a single C++ file.

When the code grows into an application, move to a project:

```bash
vix new api
cd api
vix dev
```

## Next step

Understand the role of Vix.cpp in the C++ ecosystem.

Next: [What is Vix.cpp?](/getting-started/what-is-vixcpp)
