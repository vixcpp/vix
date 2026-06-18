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

---

Vix.cpp gives C++ projects a modern application workflow while keeping native performance, explicit control, and compatibility with the existing C++ ecosystem.

It is not only a web framework.

Vix.cpp is an application runtime and developer platform for modern C++:

```txt
C++ source code
  -> Vix.cpp workflow
  -> CMake/Ninja when needed
  -> native executable or library
```

It provides one command surface for creating projects, running files, building targets, testing, formatting, managing dependencies, packaging applications, exposing OpenAPI documentation, and operating production services.

## Why Vix.cpp exists

C++ is powerful, portable, mature, and fast.

The hard part is often not the language itself. The hard part is everything around the application:

- project structure
- build configuration
- dependency setup
- development commands
- test execution
- diagnostics
- packaging
- runtime configuration
- OpenAPI documentation
- deployment
- production logs
- health checks
- service management

Most serious C++ projects eventually rebuild the same foundation.

Vix.cpp exists to make that foundation repeatable.

The goal is not to hide C++.

The goal is to make real C++ applications easier to create, run, test, package, document, and operate.

## Quick start

Create `server.cpp`:

```cpp
#include <vix.hpp>

int main()
{
    vix::App app;

    app.get("/", [](vix::Request &, vix::Response &res) {
        res.text("Hello from Vix.cpp");
    });

    app.run();

    return 0;
}
```

Run it:

```bash
vix run server.cpp
```

Open:

```txt
http://localhost:8080
```

## OpenAPI in one command

For projects that expose API documentation, enable docs during a run:

```bash
vix run api --docs
```

Then open:

```txt
http://localhost:8080/docs
```

Or fetch the generated OpenAPI document:

```txt
http://localhost:8080/openapi.json
```

Docs are disabled by default for normal runs:

```bash
vix run api
```

This keeps API documentation explicit and environment-aware.

## Add OpenAPI routes manually

For custom applications, register the OpenAPI and documentation routes in code:

```cpp
#include <vix.hpp>
#include <vix/openapi/register_docs.hpp>

int main()
{
    vix::App app;

    app.get("/status", [](vix::Request &, vix::Response &res) {
        res.text("OK");
    });

    vix::openapi::register_openapi_and_docs(
        *app.router(),
        "Example API",
        "1.0.0"
    );

    app.run();

    return 0;
}
```

This registers:

```txt
GET /openapi.json
GET /docs
GET /docs/
GET /docs/index.html
GET /docs/swagger-ui.css
GET /docs/swagger-ui-bundle.js
```

The documentation UI is served locally with embedded Swagger UI assets.

## Document routes with metadata

OpenAPI metadata can be attached to routes through route documentation.

```cpp
vix::router::RouteDoc doc;

doc.summary = "Service status";
doc.description = "Returns whether the service is running.";
doc.tags = {"system"};
doc.responses["200"] = {
    {"description", "OK"}
};
```

The OpenAPI generator builds the document from router metadata and registered module documentation.

```txt
router routes + registry docs
  -> OpenAPI 3.0.3 document
```

## Project workflow

Create a new project:

```bash
vix new api
cd api
vix install
vix dev
```

Build the project:

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

Validate the project:

```bash
vix check --tests
```

Package it:

```bash
vix pack
```

## What Vix.cpp provides

Vix.cpp combines three layers.

### 1. Developer workflow

Vix gives C++ projects a consistent command surface:

```bash
vix run main.cpp
vix new api
vix install
vix dev
vix build
vix tests
vix fmt
vix check
vix pack
```

Instead of wiring every project manually, Vix gives the common development lifecycle a predictable shape.

### 2. Runtime modules

Vix provides reusable runtime modules for application development:

```txt
agent        async        cache        cli          conversion
core         crypto       db           env          error
fs           game         io           json         kv
log          middleware   net          orm          os
p2p          p2p_http     path         process      reply
sync         template     tests        threadpool   time
utils        validation   webrpc       websocket
```

These modules are designed to support real applications, not only small examples.

### 3. Native C++ integration

Vix does not replace the C++ toolchain.

It works with native C++, CMake, Ninja, compilers, linkers, and normal executable or library outputs.

```txt
C++:
  language, performance, native binaries

CMake and Ninja:
  build system and ecosystem compatibility

Vix.cpp:
  application workflow, runtime modules, CLI, diagnostics, project lifecycle
```

## What you can build

Vix.cpp can be used for:

- backend services
- HTTP APIs
- OpenAPI-documented APIs
- WebSocket applications
- command-line tools
- reusable C++ libraries
- local-first applications
- P2P systems
- AI agent workflows
- game-oriented projects
- production services

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

```txt
https://vixcpp.com/install
```

## Run a single C++ file

Single-file mode is the fastest way to try Vix.cpp.

Create `main.cpp`:

```cpp
#include <vix.hpp>

int main()
{
    vix::print("Hello from Vix.cpp");
    return 0;
}
```

Run it:

```bash
vix run main.cpp
```

Vix detects the source file, builds it with the native C++ toolchain, then runs the generated program.

Use this mode for:

- experiments
- examples
- small tools
- learning Vix APIs
- testing short C++ snippets

When the code grows into multiple files, tests, dependencies, or a stable application, move to a Vix project.

## Create a project

Create a new application:

```bash
vix new hello --app
cd hello
vix build
vix run
```

A generated project usually looks like this:

```txt
hello/
├── src/
│   └── main.cpp
├── tests/
│   └── test_basic.cpp
├── .env.example
├── vix.app
├── vix.json
└── README.md
```

This structure is intentionally small.

It gives the application enough organization to grow without forcing a complex layout too early.

## Development mode

Use `vix dev` during active development:

```bash
vix dev
```

The development loop is:

```txt
edit
save
detect change
rebuild
restart
continue
```

Use:

```bash
vix build
```

when you only want to compile.

Use:

```bash
vix run
```

when you want to start the application manually.

Use:

```bash
vix dev
```

when you are actively editing code.

## Build system model

Vix supports two project models:

```txt
CMakeLists.txt
vix.app
```

Resolution order:

```txt
1. CMakeLists.txt
2. vix.app
```

If `CMakeLists.txt` exists, Vix uses it.

If no `CMakeLists.txt` exists but `vix.app` exists, Vix loads the manifest and generates an internal CMake project.

```txt
Project with CMakeLists.txt
  -> Vix uses the existing CMake project

Project with vix.app
  -> Vix generates an internal CMake project
  -> Vix builds the target
```

This keeps advanced CMake projects in control while giving simpler applications a cleaner manifest workflow.

## Build examples

Build the current project:

```bash
vix build
```

Build with verbose output:

```bash
vix build -v
```

Build a release version:

```bash
vix build --preset release
```

Build a specific target:

```bash
vix build --build-target vix
```

Build everything:

```bash
vix build --build-target all
```

Build a single C++ file:

```bash
vix build main.cpp
```

## Run targets

`vix run` is not limited to web applications.

It can run:

- the current project
- a named project or target
- a single C++ file
- a `.vix` manifest
- a compiled binary
- a Docker image
- a container image
- an SSH target
- an HTTP or HTTPS target
- Vix umbrella examples

Examples:

```bash
vix run
vix run api
vix run main.cpp
vix run app.vix
vix run ./app
vix run docker://nginx
vix run ssh://user@host
vix run https://example.com
```

## Command map

| Command            | Purpose                               |
| ------------------ | ------------------------------------- |
| `vix run main.cpp` | Run a single C++ file                 |
| `vix new <name>`   | Create a new project                  |
| `vix install`      | Install project dependencies          |
| `vix dev`          | Start development mode                |
| `vix build`        | Configure and build                   |
| `vix run`          | Build if needed, then run             |
| `vix tests`        | Run tests                             |
| `vix fmt`          | Format source files                   |
| `vix check`        | Validate the project                  |
| `vix pack`         | Package a project                     |
| `vix registry`     | Manage registry metadata              |
| `vix replay`       | Replay a recorded execution           |
| `vix agent`        | Run AI-assisted development workflows |
| `vix game export`  | Export a game project                 |

## Architecture

Vix.cpp is organized as a modular framework.

```txt
vix/
├── modules/
│   ├── agent/
│   ├── async/
│   ├── cache/
│   ├── cli/
│   ├── core/
│   ├── crypto/
│   ├── db/
│   ├── env/
│   ├── error/
│   ├── fs/
│   ├── game/
│   ├── io/
│   ├── json/
│   ├── kv/
│   ├── log/
│   ├── middleware/
│   ├── net/
│   ├── orm/
│   ├── p2p/
│   ├── p2p_http/
│   ├── path/
│   ├── process/
│   ├── reply/
│   ├── sync/
│   ├── template/
│   ├── tests/
│   ├── threadpool/
│   ├── time/
│   ├── utils/
│   ├── validation/
│   ├── websocket/
│   └── webrpc/
├── docs/
├── tests/
└── benchmarks/
```

Each module can evolve independently while still being part of the same application platform.

## HTTP server example

```cpp
#include <vix.hpp>

int main()
{
    vix::App app;

    app.get("/", [](vix::Request &, vix::Response &res) {
        res.text("Hello from Vix.cpp");
    });

    app.get("/health", [](vix::Request &, vix::Response &res) {
        res.text("OK");
    });

    app.run();

    return 0;
}
```

Run:

```bash
vix run server.cpp
```

Test:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
```

## API documentation workflow

A professional API should be inspectable.

Vix supports OpenAPI documentation routes that can be enabled explicitly.

Development run with docs:

```bash
vix run api --docs
```

Normal run without docs:

```bash
vix run api
```

Explicitly disable docs:

```bash
vix run api --no-docs
```

Environment variable form:

```bash
VIX_DOCS=1 vix run api
VIX_DOCS=0 vix run api
```

Recommended production policy:

```txt
disable docs completely
protect /docs behind auth
serve docs only in internal environments
serve /openapi.json only in CI or staging
```

## Registry and packages

Vix includes a registry workflow for package-based C++ projects.

```bash
vix registry sync
vix add <package>
vix install
```

The registry model is:

```txt
Registry index       -> package metadata
Store                -> fetched package Git checkouts
vix.json             -> declared project dependency requirements
vix.lock             -> exact resolved dependency versions
.vix/deps            -> project-local dependency links or copies
.vix/vix_deps.cmake  -> generated dependency integration
```

The registry keeps package metadata separate from the project source.

## Rix

Rix is the unified userland library layer for Vix.cpp.

Vix provides:

```txt
runtime
CLI
build workflow
registry integration
core modules
```

Rix provides:

```txt
optional userland libraries
a unified facade
independent packages
```

Install the unified facade:

```bash
vix add @rix/rix
vix install
```

Use it:

```cpp
#include <rix.hpp>

int main()
{
    rix.debug.print("Hello", "Rix");

    auto table = rix.csv.parse("name,language\nAda,C++\n");

    rix.debug.log("loaded {} rows", table.size());

    return 0;
}
```

## Production workflow

Vix is designed to make production services inspectable and easier to operate.

A production-oriented workflow can include:

```bash
vix build --preset release
vix service install
vix service start
vix service status
vix service logs
vix proxy nginx check
vix doctor production
```

The goal is to make production state visible:

```txt
App status
Binary path
Service status
HTTP port
WebSocket port
Public URL
Proxy state
TLS state
Healthcheck result
Logs
```

C++ applications should not become black boxes after deployment.

## Replay failed runs

`vix replay` helps reproduce recorded executions.

Record a run:

```bash
vix run api --replay
```

Replay the latest recorded run:

```bash
vix replay last
```

Replay the latest failed run:

```bash
vix replay failed
```

This is useful when a run failed, crashed, or behaved unexpectedly and you want to reproduce the same context.

## AI-assisted workflows

Vix includes an AI agent command for local project analysis and assisted development workflows.

```bash
vix agent ask "Explain this project"
vix agent analyze .
vix agent scan .
```

The agent can inspect a workspace when allowed, summarize project structure, and help with development tasks.

## Game projects

Vix includes game-oriented tooling through the `vix game` namespace.

```bash
vix game export
```

This can export a game project into a distributable directory with assets and metadata.

## Benchmarks

Vix.cpp treats performance as an engineering concern, not only a marketing claim.

The Core module includes an official benchmark suite used to track performance across releases and detect regressions before they are merged or released.

Benchmark source:

- [Core benchmarks](https://github.com/vixcpp/vix/tree/main/modules/core/benchmarks)
- [Core benchmarks README](https://github.com/vixcpp/vix/blob/main/modules/core/benchmarks/README.md)
- [v2.6.3 benchmark baseline](https://github.com/vixcpp/vix/tree/main/modules/core/benchmarks/baselines/v2.6.3)
- [Engineering note: Vix Core v2.6.3 benchmark baseline](https://blog.vixcpp.com/posts/vix-core/vix-core-benchmark-baseline-v263)

Example:

```bash
vix build --build-target benchmarks
```

Core benchmark areas include:

```txt
runtime.task
runtime.queue
runtime.scheduler
runtime.worker

executor.submit
executor.post
executor.metrics

router.match
router.registration

http.request
http.response

session.fake_transport

app.route_registration
app.group_registration
```

The official Core benchmark baseline is stored under:

```txt
modules/core/benchmarks/baselines/v2.6.3/
```

Baseline files include:

```txt
core_app_group_registration_bench.json
core_app_route_registration_bench.json
core_executor_metrics_bench.json
core_executor_post_bench.json
core_executor_submit_bench.json
core_http_request_bench.json
core_http_response_bench.json
core_router_match_bench.json
core_router_registration_bench.json
core_runtime_queue_bench.json
core_runtime_scheduler_bench.json
core_runtime_task_bench.json
core_runtime_worker_bench.json
core_session_fake_transport_bench.json
```

Official benchmark numbers must be generated from Release builds.

```txt
dev/debug = compile, test, debug
release   = measure performance
```

From `modules/core`, build the benchmark suite with:

```bash
vix build --clean --preset release --build-target all -v -- \
  -DVIX_CORE_BUILD_BENCHMARKS=ON \
  -DVIX_CORE_BUILD_TESTS=ON
```

Run one benchmark:

```bash
./build-release/benchmarks/core/core_router_match_bench
```

Run the full Core benchmark suite:

```bash
./scripts/run_core_benchmarks.sh \
  --bin-dir build-release/benchmarks/core \
  --out-dir benchmarks/results/dev \
  --version dev \
  --runner local \
  --machine local
```

Compare results against the official baseline:

```bash
./scripts/compare_core_benchmarks.py \
  benchmarks/baselines/v2.6.3 \
  benchmarks/results/current
```

By default, benchmark comparison uses:

```txt
median_ops_per_sec
```

Higher is better.

Default thresholds:

```txt
WARN = -5%
FAIL = -10%
```

## Engineering principles

Vix.cpp is built around these principles:

- keep native C++
- do not hide the toolchain
- integrate with CMake instead of fighting it
- make common workflows repeatable
- expose APIs through OpenAPI when needed
- keep diagnostics readable
- keep production inspectable
- make modules composable
- measure performance instead of guessing
- prefer explicit APIs over hidden magic
- make simple projects easy and complex projects possible

## What Vix.cpp is not

Vix.cpp does not replace C++.

It does not turn C++ into a scripting language.

It does not replace CMake for advanced build systems.

It does not force every project into one structure.

It does not expose documentation by default in every environment.

It gives C++ applications a consistent workflow and a modular runtime foundation while preserving native control.

## Project status

Vix.cpp is under active development.

Some areas are stable and suitable for real application work.

Some areas are still evolving and should be evaluated carefully before production use.

The project is developed with an emphasis on:

- real application workflows
- tests
- benchmarks
- OpenAPI documentation
- production diagnostics
- modular design

## Documentation

Start here:

- [Website](https://vixcpp.com)
- [Documentation](https://docs.vixcpp.com)
- [Registry](https://registry.vixcpp.com)
- [Rix](https://rix.vixcpp.com)
- [Engineering notes](https://blog.vixcpp.com)

Recommended path:

1. Install Vix.cpp
2. Run your first C++ file
3. Create your first project
4. Build your first HTTP server
5. Enable OpenAPI docs locally
6. Learn the CLI commands
7. Explore runtime modules
8. Add packages from the registry
9. Move from local development to production

## Built with Vix.cpp

| Project            | Purpose                                        |
| ------------------ | ---------------------------------------------- |
| Rix                | Unified userland library layer for Vix.cpp     |
| Cnerium            | Durable backend reliability layer built on Vix |
| Vix Game           | Game-oriented runtime project                  |
| PulseGrid          | Production-style service monitoring project    |
| Softadastra Runner | Command runner built with Vix.cpp              |

## Contributing

Contributions are welcome.

Before opening a pull request, run:

```bash
vix fmt --check
vix check --tests
vix build --preset release
```

For changes touching performance-sensitive code, run the benchmark suite and compare results with the current baseline.

## License

MIT License.
