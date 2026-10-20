<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1792504375/logo_rm5ktl.svg"
    width="120"
    alt="Vix.cpp logo"
  />
</p>

<h1 align="center">Vix.cpp</h1>

<p align="center">
  <strong>A coherent C++ programming platform.</strong>
</p>

<p align="center">
  C++20 · Native systems · Android · WebAssembly (WASI)
</p>

<p align="center">
  <a href="https://vixcpp.com">Website</a>
  ·
  <a href="https://vixcpp.com/docs">Documentation</a>
  ·
  <a href="https://vixcpp.com/news">Engineering News</a>
  ·
  <a href="https://github.com/vixcpp/vix/releases">Releases</a>
</p>

Build command-line tools, networked programs, concurrent applications, and native software with **one consistent C++ programming model**.

Vix.cpp brings together a small developer CLI, a shared Standard, and composable system Domains. You keep C++20, native compilers, CMake, and the libraries you already use. Vix provides common contracts for the things applications need to do: report failures, manage time and cancellation, work with files, launch processes, communicate over networks, and coordinate concurrent work.

## Build it in Vix.cpp

Vix is for software that needs to interact with its environment, not just compute a result.

| Build                              | What you can make                                                                                      |
| ---------------------------------- | ------------------------------------------------------------------------------------------------------ |
| **Command-line tools**             | Developer utilities, automation tools, and native command-line applications.                           |
| **Networked software**             | TCP clients, servers, local services, and custom wire protocols.                                       |
| **Concurrent applications**        | Background workers and programs that coordinate independent operations.                                |
| **System utilities and libraries** | Reusable C++ components that handle files, processes, and operating-system resources.                  |
| **Native Android components**      | C++ code for Android apps, including file operations, concurrency, and TCP networking through the NDK. |
| **WebAssembly programs**           | Capability-constrained C++ programs targeting WASI Preview 2, for execution in compatible hosts.       |

The platform does not require these applications to follow one framework structure. Start with a source file, use an existing CMake project, or create a new project with the Vix CLI.

## Why Vix?

C++ gives developers excellent control over performance and the machine. But applications quickly grow beyond language features alone. Filesystem code has one error convention, networking has another, asynchronous work introduces cancellation and lifetime rules, and each new library brings its own assumptions.

Vix addresses that fragmentation with **a few explicit, composable concepts**. A result explains whether an operation succeeded. An error carries an identity rather than depending on its message. A completion describes how an operation finished. Cancellation and deadlines have consistent meaning across the platform.

These contracts should be useful before a developer needs to understand the machinery underneath them. They also make the boundaries visible: Vix does not silently take ownership of resources or promise capabilities that an execution environment cannot provide.

## The Vix programming model

The platform has two connected parts:

**Vix Standard** establishes the shared vocabulary: `Result`, `Error`, error identities, `Completion`, cancellation, durations, instants, and deadlines.

**Vix Domains** use those concepts to provide system capabilities:

| Domain          | Responsibility                                                                   |
| --------------- | -------------------------------------------------------------------------------- |
| **Files**       | Paths, bounded reads and writes, metadata, and directory operations.             |
| **Process**     | Launching native processes, waiting for completion, and handling process output. |
| **Network**     | TCP connections, listeners, and network operations with explicit lifetimes.      |
| **Concurrency** | Work execution, notifications, and cooperative cancellation.                     |

For example, reading a file uses the same explicit error-handling approach as other Vix operations:

```cpp
#include <vix/files/Files.hpp>
#include <iostream>

int main()
{
    using namespace vix::files;

    auto path = Path::from_utf8("message.txt");
    if (!path)
    {
        std::cerr << path.error().message() << '\n';
        return 1;
    }

    auto content = read_utf8(path.value(), ReadLimit{4096});
    if (!content)
    {
        std::cerr << content.error().message() << '\n';
        return 1;
    }

    std::cout << content.value();
}
```

The read limit and failure path are explicit: the operation returns a result that the caller handles. The same design principles extend to process execution, networking, and concurrent work, with operation-specific completion and ownership contracts.

Learn the APIs and build complete examples in the [Vix documentation](https://vixcpp.com/docs).

## One model, different execution environments

Vix targets native operating systems as well as environments with different resource and permission models.

| Target                           | What Vix enables                                                                                                   | Important boundary                                                                                                                               |
| -------------------------------- | ------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Linux, macOS, Windows**        | Native C++ applications using the Standard and supported system Domains.                                           | Actual OS permissions and available facilities still apply.                                                                                      |
| **Android (NDK)**                | Native C++ components for Android applications.                                                                    | Android API 21+, `arm64-v8a` and `x86_64`. Cross-build qualification is not the same as testing on an Android device.                            |
| **WebAssembly / WASI Preview 2** | Portable, capability-constrained programs, including files and cooperative networking where the host permits them. | Targets `wasm32-wasip2`, not browser WebAssembly. Process creation is unsupported, and file/network access depends on host-granted capabilities. |

Android support does not imply a UI toolkit or APK builder. WASI support does not mean unrestricted access to the host machine. These are **C++ platform targets**, not a promise that the same CLI installer or every Domain operation is available everywhere.

## Start building

With a Vix 3 CLI installation, create a project and run it:

```sh
vix new hello
cd hello
vix build
vix run
```

Or use a single C++ source file:

```sh
vix run main.cpp
```

An existing project can adopt Vix without abandoning its native build system:

```sh
vix init
vix build
vix test
```

The CLI also provides `vix dev` for development, along with dependency commands. Use `vix help` to see the installed command surface.

For installation instructions and platform requirements, see **[vixcpp.com/docs](https://vixcpp.com/docs)** and the [official releases](https://github.com/vixcpp/vix/releases). Vix 3 installation instructions must be used for Vix 3; older Vix2 installers are not interchangeable.

## Dependencies belong to the C++ ecosystem

Vix works with native C++ libraries and keeps dependency resolution explicit. Project dependencies are tracked in `vix.lock` so the same resolved set can be materialized again, including in CI.

```sh
vix add publisher/package
vix install
vix update
```

Replace `publisher/package` with a package available in your configured Registry. `vix add` resolves a dependency, `vix install` materializes the lockfile, and `vix update` resolves newer versions. Package authors can submit packages with `vix publish` when their Registry configuration and credentials are in place. Registry acceptance does not, by itself, establish public indexing or discoverability.

The public Registry service and its website are separate from the stable Vix programming platform.

## Native C++ remains native

Vix does not introduce a new language, virtual machine, or mandatory application framework. It works with GCC, Clang, and MSVC toolchains and can integrate with existing CMake projects. The compiler, linker, build inputs, dependencies, and native outputs remain part of the developer's normal workflow.

Vix is not trying to replace everything around C++. Its purpose is to make the parts it _does_ provide feel like one coherent system.

## Vix 3 and Vix2

**Vix 3 is a new, incompatible generation.** It is not a drop-in upgrade for applications built against the historical Vix2 framework. Vix2 remains available as the [legacy v2.9.0 release](https://github.com/vixcpp/vix/releases/tag/v2.9.0), but the new platform follows a different programming model and a smaller, deliberate public API.

Vix 3 does not currently provide the former `vix repl`, Vix Note, application-module scaffolding, or deployment commands. HTTP and TLS are not part of the stable Vix 3 Domain surface; the Network Domain currently exposes native TCP capabilities.

## Learn and follow development

Start with the [Vix documentation](https://vixcpp.com/docs) for installation, programming concepts, supported Domains, CLI workflows, platform-specific guidance, and examples.

Read [Engineering News](https://vixcpp.com/news) for design decisions, engineering updates, and release notes. For issues and discussions, visit the [GitHub repository](https://github.com/vixcpp/vix). Please review [CONTRIBUTING.md](CONTRIBUTING.md) before proposing changes.

## Maintained by Softadastra

Vix.cpp is developed and maintained by
[Softadastra](https://softadastra.com), the company
fully dedicated to building and advancing the Vix platform.

Softadastra focuses on the engineering, adoption, and
long-term sustainability of Vix, while keeping the
platform 100% open source.

## License

Vix.cpp is released under the [MIT License](LICENSE).
