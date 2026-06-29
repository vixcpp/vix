<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782676478/vix_dean7s.svg"
    width="120"
    alt="Vix.cpp"
  />
</p>

<p align="center">
  <strong>Vix.cpp is a modern application workflow for native C++.</strong>
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

Vix.cpp is a runtime and developer toolkit for building real applications with modern C++. It keeps the native C++ model intact, while giving projects a more direct workflow for the work that usually surrounds the code: creating a project, running it, building it, testing it, checking it, managing dependencies, installing SDK profiles, and preparing the result for production.

The project is not an attempt to replace C++, or to create a separate language with a separate ecosystem. A Vix.cpp project is still an ordinary C++ project: it is compiled by a C++ compiler, it can work with CMake and native build tools, and it produces native executables and libraries. The purpose of Vix.cpp is to make the application workflow around C++ more consistent, not to take C++ away from the developer.

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782674916/vix-architecture_yarpd9.png"
    width="100%"
    alt="Vix.cpp architecture"
  />
</p>

## What this repository is useful for

If you want to use Vix.cpp, you probably do not need to read every module in this repository. Install the CLI, install the SDK profile that matches your project, and start with the documentation.

If you want to participate in Vix.cpp development, this repository contains the CLI, runtime, modules, SDK profiles, registry workflow, tests, examples, release infrastructure, and documentation source. It is the main development repository for the Vix.cpp platform.

Finally, if you want to understand the design direction of the project, the roadmap below is more important than any single feature. Vix.cpp is not meant to be a collection of unrelated tools. Its design decisions are intended to reinforce the same goal: make native C++ application development more direct, more repeatable, and easier to trust over time.

## Installation

Vix.cpp is installed in two steps. The install script bootstraps the CLI, and the CLI installs the SDK profile you need.

Linux and macOS:

`curl -fsSL https://vixcpp.com/install.sh | bash`

Windows PowerShell:

`irm https://vixcpp.com/install.ps1 | iex`

Then inspect and install a SDK profile:

`vix upgrade --sdk list`

`vix upgrade --sdk info web`

`vix upgrade --sdk web`

## Vix CLI

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782677397/vix-cli_igxm7y.png"
    width="100%"
    alt="Vix.cpp CLI commands"
  />
</p>

The Vix CLI is the main entry point into the Vix.cpp workflow. It brings project creation, build, run, development mode, tests, diagnostics, dependency management, packaging, deployment, SDK upgrades, and local tooling under one command surface.

This does not make Vix.cpp a shell around random scripts. The CLI is part of the platform: it understands Vix projects, native C++ files, SDK profiles, package state, diagnostics, production workflows, and the development lifecycle around real applications.

Use `vix --help` to see the available commands, or `vix help <command>` for command-specific documentation.

## What's different about this project?

C++ already has mature compilers, strong performance, native binaries, a large ecosystem, and decades of production use. The difficult part is often not whether C++ can build something, but how much project wiring is required before the code feels like an application.

A small C++ program can quickly become a set of repeated decisions: how the project is laid out, how the build is configured, how the binary is run, how tests are executed, how dependencies are added, how diagnostics are read, how development mode works, how the project is packaged, and how the same workflow is reproduced on another machine or in CI.

Vix.cpp exists in that space. It gives those concerns a standard workflow while still leaving the native C++ toolchain visible and usable. It is a layer of application workflow, runtime support, diagnostics, and packaging around ordinary C++ projects.

## What it is not

Vix.cpp is not a successor to C++, and it is not a language experiment. It does not introduce a competing syntax, a separate compiler model, or a closed ecosystem that replaces the tools C++ developers already use.

It is also not a replacement for CMake. Projects with an existing CMake structure can keep using it. Projects that do not need custom CMake logic can start with a simpler Vix application manifest and still end up with native C++ build outputs.

## What it is

Vix.cpp is a way to make C++ projects behave more like applications from the beginning. A project can be created, run, built, tested, checked, packaged, and upgraded through one command surface, while still being grounded in native C++.

It also provides runtime building blocks for applications, but the README is not the place to list every module or every API. The detailed command reference, module guides, examples, SDK profiles, internals, and design notes belong in the documentation and engineering blog.

## Roadmap

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782674917/vix-roadmap_x1wuxm.png"
    width="100%"
    alt="Roadmap of where Vix.cpp design decisions lead"
  />
</p>

The roadmap shows how the main design choices fit together. Vix.cpp starts from native C++ workflow, application-first runtime design, CMake interoperability, readable diagnostics, SDK profiles, registry workflow, CI, tests, documentation, and release quality. These are not independent features. They are meant to support the same direction.

The long-term goal is not to make Vix.cpp larger for its own sake. The goal is to make the existing platform more coherent, better tested, easier to install, easier to validate, and more predictable for real C++ projects.

## Post-v2.7 direction

Vix.cpp v2.7 marks an important foundation point for the project. After this release, the priority is not to keep adding new modules or expanding the surface area of the platform.

The next phase is focused on improving what already exists: module quality, registry workflow, SDK installation, diagnostics, tests, CI coverage, release quality, examples, documentation, and real application validation.

This is not because Vix.cpp is waiting to become stable. Vix.cpp is already usable, and it is already being validated through real applications. The next phase is about maturity: fewer unnecessary additions, more depth, better maintenance, and stronger confidence in the platform.

New features can still happen, but they should earn their place. A feature belongs in Vix.cpp when it improves the existing workflow, strengthens an existing module, or solves a practical problem that matters for real C++ applications.

## Vix Note

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782674915/vix-note_ly72av.png"
    width="100%"
    alt="Vix Note"
  />
</p>

Vix Note is part of the Vix.cpp developer experience. It gives C++ developers a visual space for executable notes, examples, experiments, and diagnostics, while still using the normal Vix workflow underneath.

Start it with:

```bash
vix note
```

## Pico

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1782676705/pico_vt6jxz.png"
    width="100%"
    alt="Pico"
  />
</p>

Pico is a real application built with Vix.cpp.

It exists to keep the runtime honest. Instead of validating Vix.cpp only through isolated examples, Pico brings the pieces together inside a working application: routing, middleware, persistence, diagnostics, runtime behavior, and the developer workflow around them.

This matters because Vix.cpp is not meant to be a collection of disconnected modules. The goal is for the platform to hold together when used to build real software.

Pico is where design decisions meet application reality.

Learn more: [pico.vixcpp.com](https://pico.vixcpp.com)

## Projects around Vix.cpp

Vix.cpp is the foundation. Some projects sit above it, some validate it, and some extend it into more specialized application layers.

**[Rix](https://rix.vixcpp.com)** is the unified userland library layer for Vix.cpp. Vix provides the runtime, CLI, build workflow, registry integration, and core foundations; Rix provides optional userland packages and a cleaner facade for application-level libraries.

**[Cnerium](https://github.com/softadastra/cnerium)** is a reliability-first backend layer for Vix. It belongs above the core runtime, where application reliability, backend structure, and production-oriented patterns can evolve without turning Vix itself into a large opinionated framework.

**[Kordex](https://github.com/softadastra/kordex)** is a JavaScript runtime for reliable local-first applications, built with Vix.cpp. It shows how Vix can be used as the native foundation for higher-level runtimes and local-first application platforms.

These projects are part of the same direction: Vix.cpp should remain the native foundation, while higher-level libraries and runtimes can grow around it without making the core project lose focus.

## Building from source

To work on Vix.cpp itself, clone the repository with submodules and build it with the normal CMake workflow or with the Vix CLI. The full build instructions, platform notes, SDK profiles, and test workflows are documented in the project documentation.

## Contributing

Contributions are welcome, but changes should be focused. Vix.cpp is now large enough that quality matters more than adding surface area. Fixes, tests, documentation, diagnostics, registry improvements, CI improvements, and careful improvements to existing modules are especially valuable.

For larger changes, open an issue or discussion first.

See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md), [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md), and [CHANGELOG.md](CHANGELOG.md) for more information.

## Maintained by Softadastra

Vix.cpp is maintained by Softadastra.

Softadastra builds tools that simplify modern C++ development.

## Additional resources

- **[Vix.cpp Docs](https://docs.vixcpp.com):** official guides, command reference, SDK profiles, module documentation, examples, and internals for Vix.cpp.
- **[Vix Registry](https://registry.vixcpp.com):** package index for discovering and using reusable C++ libraries in Vix projects.
- **[Rix](https://rix.vixcpp.com):** unified userland library layer for Vix.cpp projects.
- **[Engineering Notes](https://blog.vixcpp.com):** technical articles about Vix.cpp design decisions, runtime work, releases, benchmarks, and project direction.
- **[Softadastra](https://softadastra.com):** the company maintaining Vix.cpp and building tools for modern C++ development.

## License

MIT License. See [LICENSE](LICENSE) for details.
