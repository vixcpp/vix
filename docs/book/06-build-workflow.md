# Build Workflow

A build is not only compilation.

A build is the process that turns a Vix application into something executable, testable, packageable, and deployable.

The simple command is:

```bash id="t5a1fm"
vix build
```

But the workflow behind it is bigger:

```txt id="x9ae0g"
resolve project
  -> configure
  -> compile
  -> link
  -> cache when safe
  -> report result
```

The build workflow must be fast.

But it must be correct first.

## The rule

The main rule is:

```txt id="g8yh74"
correct first
fast second
```

A build tool must never say that a target is up to date when it cannot prove it.

Speed is useful only when trust is preserved.

So the Vix build model is:

```txt id="naw1x9"
reuse only when safe
rebuild when needed
fallback when uncertain
```

That rule matters everywhere.

## What `vix build` means

When you run:

```bash id="l5b6vj"
vix build
```

you are asking Vix to:

```txt id="qovj7n"
find the current project
resolve its build input
configure it if needed
build the correct target
reuse valid cache when possible
print useful diagnostics
```

The command should work for:

```txt id="umon01"
CMake projects
vix.app projects
apps created from templates
projects with registry dependencies
projects using Vix modules
```

The user should not need to remember low-level build details for normal workflows.

## Project resolution

Before building, Vix resolves the project.

Resolution order:

```txt id="vbcku0"
1. CMakeLists.txt
2. vix.app
```

If `CMakeLists.txt` exists, Vix uses the CMake project.

If there is no `CMakeLists.txt` and `vix.app` exists, Vix uses the application manifest.

That means the build workflow supports both:

```txt id="d6ksnd"
advanced CMake projects
simple vix.app applications
```

## Building a `vix.app`

For applications, `vix.app` is the simple path.

Example:

```txt id="hqszzh"
name = "api"
type = "executable"
cpp_standard = "23"

sources = [
  "src/main.cpp",
  "src/app/AppFactory.cpp",
  "src/routes/HealthRoutes.cpp"
]

include_dirs = [
  "src"
]

modules = [
  "core",
  "json",
  "http"
]
```

When Vix builds this, the workflow is:

```txt id="f51xm8"
read vix.app
  -> validate manifest
  -> generate internal CMakeLists.txt
  -> configure generated project
  -> build target
```

The generated project is written under:

```txt id="lav6nc"
.vix/generated/app/
```

The user works with `vix.app`.

Vix uses generated CMake internally.

This gives a simple application model without losing the power of CMake.

## Building a CMake project

If the project already has `CMakeLists.txt`, Vix keeps that path.

The workflow is:

```txt id="kbh8m2"
read CMakeLists.txt
  -> use preset or default config
  -> configure
  -> build
```

This is the right path for projects that need:

```txt id="sy136x"
custom CMake logic
multiple targets
platform-specific rules
external native libraries
advanced linking
install rules
```

Vix does not remove CMake.

It gives CMake projects a better workflow.

## Build presets

A build can use presets.

Examples:

```bash id="jj8u85"
vix build --preset dev
vix build --preset release
```

A development preset should favor fast iteration.

A release preset should favor final output.

The mental model:

```txt id="hy32eu"
dev = fast local feedback
release = production-ready build
```

A project can also use `CMakePresets.json` when advanced CMake configuration is needed.

## Default build

The default build should be enough for normal development:

```bash id="c7tju8"
vix build
```

A developer should not need to pass many flags for the common case.

The command should detect the project, prepare the build directory, and build the app.

For many projects, the expected flow is:

```bash id="ejv09v"
vix install
vix build
vix run
```

## Build directories

Build output should not pollute source code.

Common directories:

```txt id="jdqyk0"
build/
build-ninja/
build-release/
.vix/
```

The exact folder can depend on the selected workflow or preset.

Generated Vix state belongs in:

```txt id="ojgjf1"
.vix/
```

Package output belongs in:

```txt id="ryz2o3"
dist/
```

The source tree should stay readable.

## Dependencies before build

If the project uses registry dependencies, install them before building.

```bash id="yfgpw2"
vix install
vix build
```

After cloning a project:

```bash id="z50eqq"
git clone https://github.com/example/api.git
cd api
vix install
vix build
```

`vix install` reads `vix.lock`.

It installs exact versions.

It does not update dependencies.

That distinction matters:

```txt id="r0xmqj"
vix install = reproduce locked state
vix update = change locked state
```

## Modules before build

A `vix.app` can declare modules:

```txt id="yr8g03"
modules = [
  "core",
  "json",
  "http",
  "db"
]
```

These modules affect the build.

If the app says it needs `db`, the build must include database support.

If the app says it needs `http`, the HTTP module must be available and linked.

The build should not guess hidden modules.

The app should declare what it needs.

## Source files

The build must know what files belong to the target.

In `vix.app`, that means:

```txt id="st4v72"
sources = [
  "src/main.cpp",
  "src/app/AppFactory.cpp",
  "src/routes/HealthRoutes.cpp"
]
```

In CMake, that means the source list is defined by `CMakeLists.txt`.

The rule:

```txt id="e0qbmk"
vix.app declares app sources directly
CMakeLists.txt controls advanced source rules
```

Both paths must produce a clear target.

## Include directories

A simple app can declare include directories:

```txt id="js7i13"
include_dirs = [
  "src",
  "include"
]
```

This lets code use clean includes:

```cpp id="2u0p1x"
#include "app/AppFactory.hpp"
#include "routes/HealthRoutes.hpp"
```

The include model should stay explicit.

Do not depend on accidental include paths.

## Build target

A Vix app should have a clear target name.

In `vix.app`:

```txt id="xy9jbc"
name = "api"
```

That name becomes the application target.

When building:

```bash id="vgksx0"
vix build
```

Vix should know which target to build.

In CMake projects, target resolution can come from the CMake project or user options.

The target must not be ambiguous in normal app workflows.

## Configure step

Some builds need configuration before compilation.

Configuration is needed when:

```txt id="pqgskv"
build directory does not exist
CMakeLists.txt changed
CMakePresets.json changed
vix.app changed
vix.json changed
vix.lock changed
*.cmake changed
toolchain changed
build preset changed
```

A configure step prepares the build system.

A compile step builds code.

Do not confuse them.

```txt id="9cqvv4"
configure = prepare build graph
compile = build source files
```

## Reconfigure vs rebuild

Not every change needs the same response.

A source change usually needs rebuild only.

```txt id="jzw4cv"
src/main.cpp -> rebuild
```

A header change usually needs rebuild only.

```txt id="qqxnmb"
include/App.hpp -> rebuild
```

A config change usually needs reconfigure and rebuild.

```txt id="ybv2q6"
CMakeLists.txt -> reconfigure and rebuild
vix.app -> reconfigure and rebuild
vix.lock -> reconfigure and rebuild
```

This is also the model used by development mode.

## Change classification

For development builds, Vix classifies changes.

Ignored paths:

```txt id="ecln7l"
.git
.vix
build
build-dev
build-ninja
build-release
node_modules
.cache
.idea
.vscode
```

Source files:

```txt id="j3ruxd"
.cpp
.cc
.cxx
.c
```

Header files:

```txt id="x82s11"
.hpp
.hh
.hxx
.h
.ipp
```

Config files:

```txt id="cwe5rl"
CMakeLists.txt
CMakePresets.json
vix.json
vix.toml
vix.lock
*.cmake
```

The result is one of:

```txt id="eisrp1"
ignore
rebuild-only
reconfigure-and-rebuild
```

That is the practical build model.

## Object cache

Object cache exists to avoid recompiling work that is already valid.

The idea:

```txt id="v8ej6t"
same source
same flags
same compiler
same relevant inputs
  -> reuse object
```

If any important input changes, the object must not be reused.

Important inputs can include:

```txt id="hyp7a6"
source file content
included headers
compiler path
compiler version
compile flags
defines
include paths
target platform
build mode
```

Object cache is useful only when correctness is protected.

## Artifact cache

Artifact cache is bigger than object cache.

It can reuse build outputs when the complete input state is still valid.

The idea:

```txt id="h0lqmd"
same target input state
  -> same artifact
  -> reuse safely
```

Artifacts can include:

```txt id="ge8lje"
executables
libraries
generated outputs
packaged build outputs
```

But again, the rule is strict:

```txt id="1uegj8"
reuse only when the input identity is proven
```

A wrong cache hit is worse than a slow build.

## Build graph

A build graph describes what depends on what.

Example:

```txt id="p60fvf"
main.cpp
  -> AppFactory.hpp
  -> Config.hpp
  -> HealthRoutes.hpp
```

At target level:

```txt id="u15hao"
api
  -> object files
  -> linked libraries
  -> generated files
```

A build graph helps Vix answer:

```txt id="x9zk34"
What changed?
What depends on it?
What must rebuild?
What can be reused?
```

This is the base of fast target builds.

## Target-aware builds

A target-aware build does not rebuild everything blindly.

It asks:

```txt id="7ksytm"
Which target was requested?
Which files affect that target?
Which tasks are dirty?
```

If only one target is requested, Vix should focus on that target.

This matters in large projects.

A backend with many modules should not rebuild unrelated work when the target does not need it.

The rule:

```txt id="ws1xri"
build what the requested target needs
do not rebuild unrelated work
```

## Ninja integration

Vix can use Ninja through CMake.

The model:

```txt id="fe900o"
Vix resolves the project
CMake generates the build graph
Ninja executes build tasks
Vix improves the workflow around it
```

Vix does not need to replace Ninja.

Ninja is already good at fast builds.

Vix adds application awareness, dependency workflows, diagnostics, and higher-level commands.

## CMake fallback

Fallback is important.

If Vix cannot safely use a direct path, it should use the safer build path.

For scripts:

```bash id="aasg6g"
vix run server.cpp
```

Vix can compile directly when enough.

But when the file needs modules, dependencies, sanitizers, or project-level config, CMake fallback is safer.

The same rule applies to build workflows:

```txt id="a7kshm"
direct when safe
fallback when needed
```

## Clean builds

Sometimes you need to remove local generated state.

Use:

```bash id="oinrzc"
vix clean
```

This removes project-local cache directories such as:

```txt id="p69xww"
.vix/
build/
```

It does not remove global Vix state under:

```txt id="meagsu"
~/.vix/
```

Use clean when the local project state is wrong or stale.

## Reset builds

A reset is stronger than clean.

```bash id="nndz3n"
vix reset
```

It runs:

```txt id="hikn6k"
vix clean
vix install
```

Use reset when dependency state or generated project state is broken.

Typical workflow:

```bash id="rg5kc5"
vix reset
vix build
```

## Build after dependency changes

When dependencies change, rebuild the project.

Examples:

```bash id="tpqv18"
vix add softadastra/json
vix build
```

```bash id="kq38x7"
vix update --install
vix build
```

```bash id="i291g6"
vix remove softadastra/json
vix install
vix build
```

Dependency changes can affect include paths, link targets, modules, and generated integration files.

So the build must treat dependency state as an input.

## Build and tests

A build only proves that the code compiles.

It does not prove behavior.

Use:

```bash id="brgzd6"
vix check --tests
```

or:

```bash id="cmln19"
vix tests
```

A serious workflow should be:

```bash id="xf9gr7"
vix build
vix check --tests
```

For release:

```bash id="y8gs8a"
vix build --preset release
vix check --tests
```

## Build and formatting

Formatting is not part of compilation.

But it belongs in the development workflow.

Use:

```bash id="pq9kjq"
vix fmt
```

Check formatting before commit:

```bash id="dxi2ez"
vix fmt --check
```

A clean validation flow:

```bash id="c743ge"
vix fmt --check
vix build
vix check --tests
```

## Build and packaging

Packaging should happen after a successful build.

```bash id="yu6au1"
vix build --preset release
vix check --tests
vix pack
vix verify
```

`vix pack` creates a distributable package.

`vix verify` checks package integrity.

Do not package broken builds.

## Build and production

Production builds should be explicit.

Use:

```bash id="d4wm84"
vix build --preset release
```

Then validate:

```bash id="m4gme3"
vix check --tests
```

Then deploy:

```bash id="as7xdi"
vix deploy
```

A production deployment can also run its own configured build command.

Example production config can point to:

```txt id="grllks"
vix build --preset release
```

The deployment workflow should not rely on an accidental debug build.

## Debug builds vs release builds

Development builds favor speed and diagnostics.

Release builds favor optimized output.

Simple model:

| Build   | Purpose                     |
| ------- | --------------------------- |
| dev     | fast local work             |
| debug   | debugging and symbols       |
| release | optimized production output |

Use the right build for the right stage.

Do not benchmark debug builds.

Do not deploy accidental local builds.

## Sanitizer builds

Some builds need sanitizers.

Examples:

```txt id="e33f2v"
address sanitizer
undefined behavior sanitizer
thread sanitizer
leak sanitizer
```

Sanitizers are useful for finding bugs.

They can affect flags, linking, and runtime behavior.

That is why scripts or projects using sanitizer modes may need a safer CMake-backed path.

The principle:

```txt id="2bfgnp"
sanitizer builds must be explicit and reproducible
```

## Build diagnostics

Good build diagnostics should show:

```txt id="laxgoe"
project kind
build directory
preset
target
compiler
generator
cache status
configured or reused
number of sources
errors with useful context
```

A failed build should not leave the developer guessing.

Bad output:

```txt id="z5tly5"
build failed
```

Better output:

```txt id="0v2a3w"
error: build failed
target: api
preset: dev
step: compile
file: src/routes/HealthRoutes.cpp
fix: run vix build -v for details
```

A build tool should help you continue.

## Verbose build

Verbose mode is useful when normal output is not enough.

Example:

```bash id="d8k85v"
vix build -v
```

Verbose output can show:

```txt id="amkrql"
resolved project kind
source count
header count
compile tasks
imported compile commands
Ninja tasks
cache hits
cache misses
linker
launcher
jobs
```

Use verbose mode for debugging, not every normal build.

## Build failure categories

Most build failures fall into a few categories:

```txt id="6m6nh5"
project resolution failed
configuration failed
dependency missing
compile error
link error
generated file missing
toolchain missing
cache invalid
runtime target missing
```

The fix depends on the category.

Examples:

```bash id="isq4s7"
vix doctor
vix info
vix install
vix reset
vix build -v
```

Use `vix doctor` for environment issues.

Use `vix info` for local Vix state.

Use `vix reset` when project-local state is broken.

## Build and `vix dev`

`vix dev` uses the build workflow repeatedly.

It watches changes, classifies them, then rebuilds or reconfigures.

The model:

```txt id="y5jwzj"
file changed
  -> classify
  -> rebuild-only or reconfigure-and-rebuild
  -> restart app
```

That means `vix dev` depends on a correct build model.

If build invalidation is wrong, dev mode becomes unreliable.

## Build and `vix run`

`vix run` can trigger a build before execution.

For a project:

```bash id="voykz2"
vix run
```

The workflow can be:

```txt id="8f4unc"
resolve project
  -> build target if needed
  -> run target
```

For a single file:

```bash id="iudwpc"
vix run main.cpp
```

The workflow can be direct compile or fallback.

The build system supports runtime.

Runtime depends on build correctness.

## Build and `vix.app`

The `vix.app` build path should be invisible enough to be simple, but visible enough to debug.

The user should know:

```txt id="kk2gm8"
vix.app was detected
internal CMake project was generated
target name was resolved
build directory was used
```

The generated CMake file is an implementation detail.

But when build fails, Vix should show where it generated the project.

That helps debugging.

## Common workflows

Install and build:

```bash id="xxt2k7"
vix install
vix build
```

Build release:

```bash id="kz81f4"
vix build --preset release
```

Clean and rebuild:

```bash id="mq7s5y"
vix clean
vix build
```

Reset and rebuild:

```bash id="h4f2c6"
vix reset
vix build
```

Build then test:

```bash id="x14qlh"
vix build
vix check --tests
```

Release validation:

```bash id="n6k8ne"
vix fmt --check
vix build --preset release
vix check --tests
vix pack
vix verify
```

## Common mistakes

### Building before installing dependencies

Wrong after clone:

```bash id="p3i10m"
vix build
```

Correct:

```bash id="kqwdgp"
vix install
vix build
```

### Using update when install is needed

Wrong after clone:

```bash id="btfo6w"
vix update
```

Correct:

```bash id="us0f87"
vix install
```

### Deploying without release build

Wrong:

```bash id="cv3frb"
vix deploy
```

Better:

```bash id="m01da6"
vix build --preset release
vix check --tests
vix deploy
```

### Trusting cache blindly

If something looks wrong:

```bash id="vg44vy"
vix clean
vix build
```

If dependency state is also suspicious:

```bash id="x81nws"
vix reset
vix build
```

### Editing build config during dev and expecting rebuild only

Config changes require reconfigure.

Examples:

```txt id="vzatwc"
vix.app
CMakeLists.txt
CMakePresets.json
vix.lock
```

These are not normal source changes.

## Build checklist

A good Vix build should answer:

```txt id="tzo7j1"
Which project was resolved?
Was it CMake or vix.app?
Which target was built?
Which preset was used?
Were dependencies installed?
Was configuration needed?
Was cache used?
What compiler was used?
Where is the output?
What failed if it failed?
```

If a build cannot answer these questions, the output is not good enough.

## What you should remember

Build workflow is not only compilation.

It is:

```txt id="k8gpi7"
resolve
  -> configure
  -> compile
  -> link
  -> cache safely
  -> report clearly
```

For apps:

```txt id="5rk6ah"
vix.app gives the simple build model
CMakeLists.txt gives the advanced build model
```

For correctness:

```txt id="bst1p3"
reuse only when safe
fallback when uncertain
```

For daily work:

```bash id="af6akd"
vix build
```

For release:

```bash id="sl8ncm"
vix build --preset release
vix check --tests
```

For broken local state:

```bash id="rmklaw"
vix reset
vix build
```

The core build model:

```txt id="sgwrro"
correct first
fast second
clear always
```

## Next chapter

[Next: Modules and Composition](/book/07-modules-and-composition)
