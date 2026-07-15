<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782676478/vix_dean7s.svg"
    width="120"
    alt="Vix.cpp logo"
  />
</p>

<h1 align="center">Vix.cpp</h1>

<p align="center">
  <strong>A modern C++ runtime for building fast and reliable applications.</strong>
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

Vix.cpp brings the work around native C++ applications into one coherent workflow. It provides a runtime, a command-line interface, SDK profiles, package management, diagnostics, testing, packaging, and production-oriented tooling without changing the language or hiding the native toolchain.

A Vix project remains an ordinary C++ project. It is compiled by a C++ compiler, can interoperate with CMake and existing build systems, and produces native executables and libraries. Vix exists to make the path from source code to a working application more direct, repeatable, and understandable.

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782674916/vix-architecture_yarpd9.png"
    width="100%"
    alt="Vix.cpp architecture"
  />
</p>

## Why Vix exists

C++ already provides mature compilers, native performance, a large ecosystem, and decades of production use. The difficult part is often everything that must be assembled around the language before a project feels like a complete application.

Even a small project quickly accumulates decisions about directory layout, build configuration, dependency resolution, test execution, development mode, diagnostics, packaging, deployment, CI, and reproducibility across machines. Each tool can solve part of that problem, but the developer is still responsible for turning those separate parts into a consistent workflow.

Vix exists to provide that workflow. It gives C++ projects a common way to be created, run, built, tested, checked, packaged, upgraded, and prepared for production while keeping the underlying compiler, build files, dependencies, and native outputs visible.

## Install Vix

Vix is installed in two stages. The install script bootstraps the CLI, then the CLI installs the SDK profile required by the project.

### Linux and macOS

```bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

### Windows PowerShell

```powershell
irm https://vixcpp.com/install.ps1 | iex
```

Confirm the installation:

```bash
vix --version
```

Then inspect the available SDK profiles and install the one that matches the application you are building:

```bash
vix upgrade --sdk list
vix upgrade --sdk info web
vix upgrade --sdk web
```

The [installation guide](https://docs.vixcpp.com) covers platform requirements, PATH configuration, SDK profiles, upgrades, and troubleshooting.

## The Vix workflow

The Vix CLI is the main entry point into the platform. It understands Vix projects, standalone C++ files, application manifests, SDK profiles, registry dependencies, build state, diagnostics, tests, and production workflows.

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782677397/vix-cli_igxm7y.png"
    width="100%"
    alt="Vix.cpp CLI commands"
  />
</p>

The command surface follows the lifecycle of an application. The same tool can create a project, run it during development, build native outputs, execute tests, inspect problems, manage packages, prepare releases, and update the installed SDK.

```bash
vix --help
vix help <command>
```

The README intentionally does not reproduce the complete command reference. Detailed command behavior, options, examples, and project formats are maintained in the [official documentation](https://docs.vixcpp.com).

## Native C++ remains visible

Vix is not a successor to C++, a new language syntax, or a separate compiler model. It does not turn C++ into an interpreted environment, and it does not place applications inside a closed runtime.

It is also not a replacement for CMake. Existing CMake projects can keep their current structure and use Vix around it. New applications that do not need custom CMake logic can begin with a simpler Vix application manifest and still produce normal native build outputs.

This distinction is central to the project. Vix improves the application workflow around C++; it does not remove the tools, formats, or knowledge that make a C++ project portable and maintainable.

## Runtime and application modules

Vix is not only a CLI wrapped around a compiler. It ships runtime modules that cover the infrastructure real native applications usually have to assemble by hand: HTTP routing, middleware, async execution, WebSocket support, configuration, environment files, filesystems, processes, databases, serialization, caching, validation, logging, testing, packaging, and diagnostics.

The important part is that these modules are designed to compose as one platform. A backend can use the HTTP runtime, middleware, database layer, validation, logging, environment loading, and tests without each piece introducing a different project model or error style. Existing CMake projects can adopt the modules directly. New Vix applications can use the app-first workflow.

For generated applications, `vix.app` is the readable source of truth at the project root. It describes the native target, C++ standard, source files, include directories, linked Vix modules, registry dependencies, compile options, resources, output directory, and enabled application modules. Vix reads that manifest, generates an internal CMake project under `.vix/generated/app/`, and still builds through the normal native toolchain. The generated files are inspectable when debugging, but the project remains driven by the manifest.

```ini
name = "api"
type = "backend"
standard = "c++20"

sources = [
  "src/main.cpp",
  "src/app/AppBootstrap.cpp",
  "src/presentation/routes/RouteRegistry.cpp",
]

include_dirs = [
  "include",
  "src",
]

packages = [
  "vix",
]

links = [
  "vix::vix",
]

[module.auth]
enabled = true
path = "modules/auth"
kind = "backend"
depends = []
```

### Git dependencies with `vix.app`

For an existing folder, `vix init` creates a minimal `vix.app` from the current project:

```bash
vix init
```

A Git dependency can then be added directly from its repository:

```bash
vix install https://github.com/fmtlib/fmt
```

Vix detects the latest stable version and the public CMake target, then records them in `vix.app`:

```toml
name = "fmt-test"
type = "executable"
standard = "c++20"
sources = ["main.cpp"]

[dependencies.fmt]
git = "https://github.com/fmtlib/fmt"
tag = "12.2.0"
target = "fmt::fmt"
```

The exact commit is preserved in `vix.lock`, and the dependency is prepared automatically when the project is built or run:

```bash
vix run main.cpp
```

For a temporary test without modifying `vix.app`:

```bash
vix run main.cpp --dep https://github.com/fmtlib/fmt
```

### Application modules

Application modules let a large Vix application remain a single native process while keeping features such as `auth`, `projects`, `billing`, `logs`, or `deployments` behind explicit public and private boundaries.

```bash
vix modules init
vix modules add auth
vix modules add realtime --websocket
vix modules check
vix build
```

Each module owns its public headers, private implementation, tests, dependencies, metadata, route prefix, and CMake target. Enabled modules are declared in `vix.app`, and Vix generates the registration and linking code required by the application.

WebSocket modules can be created with dedicated workflows:

```bash
vix modules add notifications --websocket --workflow attached
vix modules add gateway --websocket --workflow standalone
vix modules add bridge --websocket --workflow bridge
vix modules add client --websocket --workflow client
```

`vix modules check` validates module structure, explicit dependencies, enabled state, dependency cycles, duplicate route ownership, and public headers that expose private implementation paths.

See the [module documentation](https://docs.vixcpp.com), [vix.app guide](https://docs.vixcpp.com/guides/vix-app), and [application modules guide](https://docs.vixcpp.com/app-modules) for the complete reference.

## SDK profiles and the Vix Registry

SDK profiles define coherent development environments for different kinds of Vix applications. They allow the CLI, runtime modules, build configuration, and supporting tools to be installed and upgraded together instead of being assembled manually on every machine.

The [Vix Registry](https://registry.vixcpp.com) provides reusable C++ packages that can be added to applications through the Vix dependency workflow. Registry packages remain native C++ dependencies and integrate with normal project builds.

Together, SDK profiles and the registry make it easier to reproduce the same project environment locally, in CI, and across a team without turning the project into a closed ecosystem.

## Vix Note

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782674915/vix-note_ly72av.png"
    width="100%"
    alt="Vix Note interface"
  />
</p>

Vix Note provides a visual space for executable C++ notes, examples, experiments, and diagnostics. It uses the normal Vix workflow underneath, so code explored in a note remains connected to the same runtime and toolchain used by a project.

```bash
vix note
```

## Softadastra Cloud

[Softadastra Cloud](https://cloud.softadastra.com) is the product layer built around Vix for private C++ packages and team project operations. It gives C++ projects a controlled cloud workspace for the parts of development that need to be shared: workspaces, private packages, package versions, lockfiles, build reports, permissions, access tokens, public profiles, and team activity.

Vix keeps the project local. The compiler, source files, build outputs, and native workflow stay on the developer machine or inside the team’s own CI environment. Softadastra Cloud adds the shared state around that workflow, so a project can move from one developer to a team without losing visibility into what was published, which lockfile was used, which build failed, and who has access.

```bash
vix login
vix cloud init
vix cloud status
vix cloud lockfile upload
vix build --report
vix publish --cloud
```

This connection matters because Vix is not designed only from small examples. It is exercised through a real product workflow where the CLI, manifests, private package archives, lockfiles, build reports, permissions, and project metadata have to work together.

Softadastra Cloud keeps native C++ projects understandable after they leave a single machine. Developers can see what exists, what changed, what private package was published, what failed, and which parts of the project are private or public, while the local-first nature of Vix remains intact.

Use [cloud.softadastra.com](https://cloud.softadastra.com) to create a workspace, publish private packages, and connect a Vix project to the team workflow.

## Project direction

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782674917/vix-roadmap_x1wuxm.png"
    width="100%"
    alt="Vix.cpp project direction"
  />
</p>

Vix.cpp v2.7 marks an important foundation point for the project. The next phase is centered on improving what already exists rather than continuously expanding the platform with new modules.

The work now focuses on module quality, registry reliability, SDK installation, diagnostics, tests, CI coverage, release quality, examples, documentation, and validation through real applications. This direction is about maturity: fewer unnecessary additions, more depth, better maintenance, and stronger confidence in the complete workflow.

New capabilities can still be added, but they should solve a practical application problem, improve an existing workflow, or strengthen a part of the platform that developers already depend on.

## Projects around Vix.cpp

Vix.cpp remains the native foundation. Higher-level libraries and runtimes can grow around it without making the core platform lose focus.

**[Rix](https://rix.vixcpp.com)** is the optional userland library layer for Vix applications. It provides application-level packages and a unified facade above the core Vix runtime.

**[Cnerium](https://github.com/softadastra/cnerium)** is a reliability-first backend layer for Vix. It provides a place for backend structure and production-oriented patterns to evolve without turning the core runtime into a large opinionated framework.

**[Kordex](https://github.com/softadastra/kordex)** is a JavaScript runtime for reliable local-first applications built on Vix.cpp. It demonstrates how the native platform can support higher-level runtimes while preserving a C++ foundation.

## Working on this repository

This repository contains the Vix CLI, runtime, modules, SDK profiles, registry integration, tests, examples, release infrastructure, and documentation source. Users normally begin with the packaged CLI and the official documentation; contributors work directly from this repository.

To build Vix.cpp from source, clone the repository with its submodules and follow the build and test instructions in the [developer documentation](https://docs.vixcpp.com). The documented workflow covers supported platforms, build options, SDK profiles, module tests, and release checks.

## Contributing

Contributions should improve the clarity, reliability, and maintainability of the existing platform. Fixes, tests, diagnostics, documentation, registry improvements, CI work, and careful refinements to current modules are especially valuable.

For substantial changes, begin with an issue or discussion so the design can be considered in the context of the whole platform.

See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md), [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md), and [CHANGELOG.md](CHANGELOG.md) for project policies and release history.

## Maintained by Softadastra

Vix.cpp is maintained by [Softadastra](https://softadastra.com), a company building tools that simplify modern C++ development.

## Resources

- [Vix.cpp documentation](https://docs.vixcpp.com): guides, commands, SDK profiles, modules, examples, and internals.
- [Vix Registry](https://registry.vixcpp.com): reusable public packages for Vix applications.
- [Softadastra Cloud](https://cloud.softadastra.com): private C++ packages, workspaces, lockfiles, build reports, access tokens, and team project activity.
- [Rix](https://rix.vixcpp.com): optional userland libraries for Vix projects.
- [Engineering Notes](https://blog.vixcpp.com): design decisions, releases, benchmarks, and technical articles.
- [Softadastra](https://softadastra.com): the company maintaining Vix.cpp.

## License

Vix.cpp is available under the MIT License. See [LICENSE](LICENSE) for details.
