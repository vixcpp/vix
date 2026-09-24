<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782676478/vix_dean7s.svg"
    width="120"
    alt="Vix.cpp logo"
  />
</p>

<h1 align="center">Vix.cpp</h1>

<p align="center">
  <strong>A C++ developer platform for building native applications.</strong>
</p>

<p align="center">
  <a href="https://vixcpp.com">Website</a>
  ·
  <a href="https://docs.vixcpp.com">Documentation</a>
  ·
  <a href="https://registry.vixcpp.com">Registry</a>
  ·
  <a href="https://blog.vixcpp.com">Engineering Notes</a>
</p>

Vix.cpp brings the tools around a C++ application into one coherent workflow. You can use it to run and build projects, manage dependencies, work with application modules, run tests, inspect problems, package applications and prepare them for production.

Vix does not introduce a new language or hide the native C++ toolchain. A Vix project is still compiled by a C++ compiler, can work with CMake and existing libraries, and produces normal native executables and libraries.

## Why Vix exists

C++ already gives developers excellent compilers, native performance and a large ecosystem. The difficult part often starts when a program becomes an application.

A project needs a build configuration. Then dependencies, tests, development commands, diagnostics, packaging and eventually production tooling appear. There are good tools for each of these problems, but developers still have to assemble them and maintain the workflow between them.

Vix exists to make that workflow more consistent without replacing the C++ ecosystem underneath it.

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782674916/vix-architecture_yarpd9.png"
    width="100%"
    alt="Vix.cpp architecture"
  />
</p>

## Try it

Install Vix on Linux or macOS:

```bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

On Windows PowerShell:

```powershell
irm https://vixcpp.com/install.ps1 | iex
```

Check the installation:

```bash
vix --version
```

You can start with a normal C++ file.

```cpp
#include <iostream>

int main()
{
    std::cout << "Hello from Vix.cpp\n";
}
```

Run it with:

```bash
vix run main.cpp
```

There is no separate execution model here. Vix builds and runs native C++.

When the project becomes larger, the same CLI can work with complete applications:

```bash
vix init
vix run
vix build
vix tests
vix check
```

The [installation guide](https://docs.vixcpp.com) explains SDK profiles, platform requirements and the complete setup process.

## From a C++ project to an application

Vix is more than a command that invokes a compiler. The platform includes runtime modules and development tooling for the things real applications commonly need, including HTTP, middleware, asynchronous execution, WebSockets, configuration, filesystems, processes, databases, serialization, caching, validation and logging.

These parts are designed to work together instead of giving every library its own project structure and development workflow.

For Vix-managed applications, the project can be described with `vix.app`:

```ini
name = "api"
type = "backend"
standard = "c++20"

sources = [
  "src/main.cpp",
]

packages = [
  "vix",
]

links = [
  "vix::vix",
]
```

Vix reads the application description and generates the native build it needs internally. The generated files remain inspectable, and projects that need custom CMake logic can continue using CMake directly.

## Dependencies

Existing C++ libraries can be used from a Vix project.

For example:

```bash
vix install https://github.com/fmtlib/fmt
```

Vix can detect the dependency information, add it to the application and preserve the resolved commit in `vix.lock`.

A dependency can also be used temporarily without modifying the application:

```bash
vix run main.cpp --dep https://github.com/fmtlib/fmt
```

The goal is not to create a separate library ecosystem. Dependencies remain native C++ dependencies and participate in the normal build.

## Application modules

Large applications can be divided into modules while remaining a single native application.

```bash
vix modules init
vix modules add auth
vix modules add realtime --websocket
vix modules check
vix build
```

A module can own its public interface, private implementation, tests and dependencies. Vix validates the relationships between modules and generates the registration and linking code required by the application.

This gives larger C++ applications explicit boundaries without requiring every feature to become a separate service or process.

## Native C++ stays visible

Vix is not a replacement for C++, CMake or the compiler.

Existing CMake projects can keep their current structure and use Vix around them. New projects can start with `vix.app` when they do not need custom build logic.

In both cases, the important parts remain accessible to the developer: compiler diagnostics, dependencies, generated build files and native outputs.

Vix tries to remove repetitive work around C++ without making the underlying system mysterious.

## Developer tools

Vix also includes tools for working with C++ outside the normal edit, build and run cycle.

**Vix Reply** is an interactive terminal for experimenting with native C++ and inspecting compiler diagnostics.

```bash
vix repl
```

**Vix Note** is a visual workspace for executable notes, experiments and developer tooling.

```bash
vix note
```

They use the same C++ environment as the rest of the platform rather than introducing another language or runtime.

## Production

The workflow continues beyond local development.

```bash
vix deploy --dry-run
vix deploy
vix doctor production
```

Vix can help with build and test checks, service restarts, application health, production diagnostics and deployment recovery while leaving the application infrastructure under the developer's control.

## Registry

The [Vix Registry](https://registry.vixcpp.com) provides reusable C++ packages that can be installed through the Vix dependency workflow.

Packages remain normal native dependencies. Together with `vix.lock` and SDK profiles, this makes it easier to reproduce the same project environment on another machine or in CI.

## Documentation

This README is only an introduction to the project.

The [Vix.cpp documentation](https://docs.vixcpp.com) covers the application model, CLI, build workflow, runtime, modules, dependencies, SDK profiles, testing, production workflows and internal architecture in more detail.

Technical decisions, benchmarks and engineering work are published in the [Engineering Notes](https://blog.vixcpp.com).

## Contributing

Vix.cpp is developed in the open. Contributions to the runtime, modules, diagnostics, tests, documentation, registry, CI, portability and performance are welcome.

For larger changes, opening an issue or discussion first makes it easier to consider the design in the context of the whole platform.

See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md), [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) and [CHANGELOG.md](CHANGELOG.md).

## Maintained by Softadastra

Vix.cpp is maintained by [Softadastra](https://softadastra.com), a computing research and technology company.

## License

Vix.cpp is available under the MIT License. See [LICENSE](LICENSE).
