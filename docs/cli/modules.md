# vix modules

`vix modules` organizes application code into clean, explicit C++ modules.

Use it when your Vix app starts growing and you want to split code by feature, domain, or responsibility without turning the whole project into a complex CMake setup.

```bash
vix modules init
vix modules add auth
```

## Overview

`vix modules` adds an app-first module organization layer to Vix projects.

It works with:

```txt
vix.app projects
CMake projects
```

The modern recommended path for simple applications is:

```txt
vix.app
modules = [...]
```

CMake still works and remains the advanced compatibility path.

The goal is simple:

```txt
small app
clear modules
explicit dependencies
no manual CMake for common cases
CMake available when needed
```

## The important idea

For Vix apps, modules are declared in `vix.app`.

Example:

```ini
name = api
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

include_dirs = [
  src,
]

modules = [
  auth,
  users,
  orders,
]

packages = [
  vix,
]

links = [
  vix::vix,
]

output_dir = bin
```

This tells Vix:

```txt
this app uses modules/auth
this app uses modules/users
this app uses modules/orders
```

When Vix builds a `vix.app` project, it generates an internal CMake project under:

```txt
.vix/generated/app/CMakeLists.txt
```

That generated CMake project loads your modules automatically.

You do not edit the generated file.

You edit:

```txt
vix.app
```

## CMake still works

If your project already has:

```txt
CMakeLists.txt
```

Vix uses it.

Project resolution order:

```txt
1. CMakeLists.txt
2. vix.app
```

That means:

```txt
If CMakeLists.txt exists, Vix uses CMakeLists.txt.
If CMakeLists.txt does not exist but vix.app exists, Vix uses vix.app.
```

So this project uses CMake:

```txt
api/
  CMakeLists.txt
  vix.app
  modules/
  src/
```

This project uses `vix.app`:

```txt
api/
  vix.app
  modules/
  src/
```

This keeps existing CMake projects compatible while allowing new Vix apps to stay simpler.

## When to use modules

Use modules when your app has separate areas like:

```txt
auth
users
orders
billing
notifications
catalog
storage
search
```

Instead of putting everything in:

```txt
src/
```

you can use:

```txt
modules/
  auth/
  users/
  orders/
```

Each module has its own public API, private implementation, and build target.

## What a module is

A Vix module is a small C++ library inside your application.

A module named `auth` looks like this:

```txt
modules/auth/
├── CMakeLists.txt
├── include/
│   └── auth/
│       └── api.hpp
└── src/
    └── auth.cpp
```

The public header is:

```txt
modules/auth/include/auth/api.hpp
```

The private implementation is:

```txt
modules/auth/src/auth.cpp
```

The module target is:

```txt
<project>_auth
```

The public alias target is:

```txt
<project>::auth
```

For a project named `api`, the alias is:

```txt
api::auth
```

## Basic workflow with vix.app

Create an app:

```bash
vix new api --app
cd api
```

Initialize modules:

```bash
vix modules init
```

Add modules:

```bash
vix modules add auth
vix modules add users
```

Declare them in `vix.app`:

```ini
modules = [
  auth,
  users,
]
```

Use them in code:

```cpp
#include <auth/api.hpp>
#include <users/api.hpp>
```

Build:

```bash
vix build
```

Run:

```bash
vix run
```

## Basic workflow with CMake

For an existing CMake project:

```bash
vix modules init
vix modules add auth
vix build
```

In CMake projects, `vix modules init` can patch the root `CMakeLists.txt` with:

```cmake
# VIX_MODULES_BEGIN
include(${CMAKE_CURRENT_LIST_DIR}/cmake/vix_modules.cmake)
# VIX_MODULES_END
```

When a module is added, Vix can also add a guarded link block to connect the module to the main target.

This keeps existing CMake projects working.

## Why vix.app does not patch CMakeLists.txt

A `vix.app` project usually has no root `CMakeLists.txt`.

That is intentional.

When you run:

```bash
vix modules init
```

inside a `vix.app` project, Vix creates:

```txt
modules/
cmake/vix_modules.cmake
```

but skips patching `CMakeLists.txt` because the app is controlled by `vix.app`.

You should connect modules through:

```ini
modules = [
  auth,
  users,
]
```

not by editing generated CMake.

## Generated files

`vix modules init` creates:

```txt
modules/
cmake/vix_modules.cmake
```

`vix modules add auth` creates:

```txt
modules/auth/
├── CMakeLists.txt
├── include/
│   └── auth/
│       └── api.hpp
└── src/
    └── auth.cpp
```

The generated module is ready to build.

## `vix.app` modules field

The `modules` field is for internal app modules.

Example:

```ini
modules = [
  auth,
  users,
  billing,
]
```

These names map to module folders:

```txt
modules/auth
modules/users
modules/billing
```

And to CMake alias targets:

```txt
api::auth
api::users
api::billing
```

Do not use `modules` for registry packages.

Use `deps` for registry packages.

## `modules` vs `deps`

Use `modules` for code inside your app.

Use `deps` for packages from the Vix Registry.

Example:

```ini
modules = [
  auth,
  users,
]

deps = [
  gk/json@^1.0.0,
]
```

The difference is important:

| Field      | Purpose                                     | Example          |
| ---------- | ------------------------------------------- | ---------------- |
| `modules`  | Internal app modules under `modules/`       | `auth`           |
| `deps`     | External packages from the Vix Registry     | `gk/json@^1.0.0` |
| `packages` | CMake packages resolved with `find_package` | `vix`            |
| `links`    | CMake targets linked to the app             | `vix::vix`       |

## Full vix.app example with modules

```ini
name = api
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

include_dirs = [
  src,
]

modules = [
  auth,
  users,
  orders,
]

deps = [
]

packages = [
  vix,
]

links = [
  vix::vix,
]

resources = [
  .env=.env,
]

output_dir = bin
```

This is the app-first Vix structure.

The app owns the modules.

The modules stay clean and explicit.

## Full project structure

A modular `vix.app` project can look like this:

```txt
api/
├── vix.app
├── vix.json
├── .env
├── .env.example
├── src/
│   └── main.cpp
├── modules/
│   ├── auth/
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── auth/
│   │   │       └── api.hpp
│   │   └── src/
│   │       └── auth.cpp
│   ├── users/
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── users/
│   │   │       └── api.hpp
│   │   └── src/
│   │       └── users.cpp
│   └── orders/
│       ├── CMakeLists.txt
│       ├── include/
│       │   └── orders/
│       │       └── api.hpp
│       └── src/
│           └── orders.cpp
└── cmake/
    └── vix_modules.cmake
```

## Public include style

Use public include paths:

```cpp
#include <auth/api.hpp>
#include <users/api.hpp>
```

Do not use relative module paths:

```cpp
#include "../modules/auth/include/auth/api.hpp"
```

The module target exposes its public include directory.

The include should stay clean.

## Generated module header

A generated module header can look like this:

```cpp
#ifndef api_auth_api_hpp
#define api_auth_api_hpp

#include <string>

namespace api::auth
{
  struct Api
  {
    static std::string name();
  };
}

#endif
```

The exact namespace depends on the project name and module name.

## Generated module implementation

A generated module implementation can look like this:

```cpp
#include <auth/api.hpp>

namespace api::auth
{
  std::string Api::name()
  {
    return "api::auth";
  }
}
```

## Generated module CMake target

A generated module CMake file creates a real target and an alias target.

Example for project `api` and module `auth`:

```cmake
add_library(api_auth)
add_library(api::auth ALIAS api_auth)

target_sources(api_auth
  PRIVATE
    src/auth.cpp
)

target_include_directories(api_auth
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(api_auth PUBLIC cxx_std_20)

set_target_properties(api_auth PROPERTIES
  OUTPUT_NAME "api_auth"
)
```

The alias target is the public name:

```txt
api::auth
```

## Module loader

`vix modules init` creates:

```txt
cmake/vix_modules.cmake
```

The loader scans:

```txt
modules/
```

and adds every module that contains:

```txt
CMakeLists.txt
```

Conceptually:

```cmake
file(GLOB VIX_MODULE_DIRS RELATIVE "${VIX_MODULES_DIR}" "${VIX_MODULES_DIR}/*")

foreach(_m ${VIX_MODULE_DIRS})
  if(IS_DIRECTORY "${VIX_MODULES_DIR}/${_m}")
    if(EXISTS "${VIX_MODULES_DIR}/${_m}/CMakeLists.txt")
      add_subdirectory("${VIX_MODULES_DIR}/${_m}" "${CMAKE_BINARY_DIR}/vix_modules/${_m}")
    endif()
  endif()
endforeach()
```

This loader is used by both paths:

```txt
vix.app generated CMake
manual CMake project
```

## How vix.app connects modules internally

For a `vix.app` project, Vix generates internal CMake.

The generated project can include the module loader and link declared modules.

Conceptually:

```txt
vix.app
  -> reads modules = [...]
  -> generates .vix/generated/app/CMakeLists.txt
  -> includes cmake/vix_modules.cmake
  -> links module aliases to the app target
```

So this:

```ini
modules = [
  auth,
  users,
]
```

maps to:

```txt
api::auth
api::users
```

and links them into the app.

## Do not edit generated CMake

Do not edit:

```txt
.vix/generated/app/CMakeLists.txt
```

It is generated by Vix.

If you need to add a module, edit:

```ini
modules = [
  auth,
  users,
  orders,
]
```

If you need to add a registry dependency, edit or use `vix add` so the project metadata and lockfile stay aligned.

If you need full custom build logic, use a real `CMakeLists.txt`.

## Adding a module

Run:

```bash
vix modules add auth
```

This creates:

```txt
modules/auth/include/auth/api.hpp
modules/auth/src/auth.cpp
modules/auth/CMakeLists.txt
```

Then add it to `vix.app`:

```ini
modules = [
  auth,
]
```

Build:

```bash
vix build
```

## Adding multiple modules

```bash
vix modules add auth
vix modules add users
vix modules add orders
```

Then:

```ini
modules = [
  auth,
  users,
  orders,
]
```

Use in code:

```cpp
#include <auth/api.hpp>
#include <users/api.hpp>
#include <orders/api.hpp>
```

## Module naming rules

Module names may contain:

```txt
letters
numbers
_
-
```

Examples:

```txt
auth
users
order_items
payment-gateway
```

Hyphens are normalized to underscores for target names.

Example:

```txt
payment-gateway
```

becomes:

```txt
payment_gateway
```

So the alias for project `api` becomes:

```txt
api::payment_gateway
```

## Reserved module names

Avoid names that conflict with project structure, build tools, or common dependency names.

Examples of reserved or bad names:

```txt
modules
module
src
include
cmake
build
test
tests
example
examples
vendor
third_party
internal
private
public
main
app
api
core
std
vix
vixcpp
registry
deps
fmt
boost
openssl
sqlite
mysql
asio
beast
```

Use domain names instead:

```txt
auth
users
orders
billing
notifications
inventory
search
```

## `--project`

Use `--project` to set the project name used for module targets.

```bash
vix modules add auth --project api
```

This creates aliases like:

```txt
api::auth
```

Use this when Vix cannot detect the project name correctly.

## `--dir`

Use `--dir` to run module commands from another directory.

```bash
vix modules init --dir ./apps/api
vix modules add auth --dir ./apps/api
vix modules check --dir ./apps/api
```

Equivalent form:

```bash
vix modules add auth --dir=./apps/api
```

## `--no-patch`

Use `--no-patch` with `init` when you do not want Vix to patch a root `CMakeLists.txt`.

```bash
vix modules init --no-patch
```

This still creates:

```txt
modules/
cmake/vix_modules.cmake
```

but does not modify root CMake.

For `vix.app` projects, root CMake patching is skipped because `vix.app` is the source of truth.

## `--patch`

Use `--patch` to explicitly allow root CMake patching.

```bash
vix modules init --patch
```

This matters only for CMake projects with a root `CMakeLists.txt`.

## `--no-link`

Use `--no-link` when adding a module if you do not want Vix to patch the root CMake link block.

```bash
vix modules add auth --no-link
```

For `vix.app` projects, prefer declaring the module in `vix.app`:

```ini
modules = [
  auth,
]
```

For manual CMake projects, link manually:

```cmake
target_link_libraries(api PRIVATE api::auth)
```

## `--link`

Use `--link` to allow Vix to patch a CMake project link block.

```bash
vix modules add auth --link
```

This is useful for classic CMake projects.

For `vix.app`, linking is controlled by:

```ini
modules = [...]
```

## Cross-module dependencies

If one module uses another module, the dependency must be explicit.

Example:

```cpp
#include <auth/api.hpp>
```

If `users` includes `auth`, then `users` must link to `auth`.

In:

```txt
modules/users/CMakeLists.txt
```

add:

```cmake
target_link_libraries(api_users PUBLIC api::auth)
```

or:

```cmake
target_link_libraries(api_users PRIVATE api::auth)
```

The correct visibility depends on where the dependency is used.

## PUBLIC vs PRIVATE dependencies

Use `PUBLIC` when the dependency appears in a public header.

Example:

```cpp
// modules/users/include/users/api.hpp
#include <auth/api.hpp>
```

Then:

```cmake
target_link_libraries(api_users PUBLIC api::auth)
```

Use `PRIVATE` when the dependency is only used inside `.cpp` files.

Example:

```cpp
// modules/users/src/users.cpp
#include <auth/api.hpp>
```

Then:

```cmake
target_link_libraries(api_users PRIVATE api::auth)
```

## Module safety rules

Run:

```bash
vix modules check
```

This checks module structure and dependency safety.

The main rules are:

```txt
public headers must not include private implementation files
cross-module includes must be declared through target_link_libraries
module dependencies must be explicit
```

## Public headers must not include private files

Wrong:

```cpp
#include "../src/auth_impl.hpp"
```

or:

```cpp
#include <auth/src/auth_impl.hpp>
```

Public headers should expose stable public API only.

Correct:

```cpp
#include <auth/api.hpp>
```

Private implementation headers should stay under:

```txt
modules/auth/src/
```

and be included only by implementation files.

## Cross-module includes must be declared

If a module includes another module:

```cpp
#include <auth/api.hpp>
```

the CMake dependency must exist.

Example:

```cmake
target_link_libraries(api_users PUBLIC api::auth)
```

This makes the dependency visible to the build system.

It also avoids hidden include path problems.

## App code using modules

In your app source:

```cpp
#include <vix.hpp>
#include <auth/api.hpp>
#include <users/api.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "auth", api::auth::Api::name(),
      "users", api::users::Api::name()
    });
  });

  app.run();

  return 0;
}
```

Your `vix.app` must include:

```ini
modules = [
  auth,
  users,
]
```

## App-first architecture example

A real backend can evolve like this:

```txt
api/
├── vix.app
├── src/
│   └── main.cpp
├── modules/
│   ├── auth/
│   ├── users/
│   ├── orders/
│   ├── billing/
│   └── notifications/
└── cmake/
    └── vix_modules.cmake
```

Each module owns one part of the domain.

Example responsibilities:

| Module          | Responsibility                    |
| --------------- | --------------------------------- |
| `auth`          | login, tokens, permissions        |
| `users`         | user profile and user lookup      |
| `orders`        | order creation and order state    |
| `billing`       | payments, invoices, subscriptions |
| `notifications` | email, websocket, push events     |

## Recommended module boundaries

A module should own one clear responsibility.

Good module names:

```txt
auth
users
orders
billing
notifications
catalog
inventory
search
files
storage
```

Avoid generic names:

```txt
utils
helpers
core
common
misc
```

Generic modules become dumping grounds.

Prefer domain modules.

## Keep main.cpp small

Do not put every route and service directly in `main.cpp`.

Prefer:

```cpp
#include <vix.hpp>
#include <auth/api.hpp>
#include <users/api.hpp>

using namespace vix;

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res) {
    res.text("ok");
  });
}

int main()
{
  App app;

  register_routes(app);

  app.run();
  return 0;
}
```

As the app grows, modules should own their own route registration functions.

Example:

```cpp
auth::register_routes(app);
users::register_routes(app);
```

## Module route registration pattern

A module can expose a route registration API.

Example:

```cpp
// modules/auth/include/auth/api.hpp
#ifndef api_auth_api_hpp
#define api_auth_api_hpp

#include <vix.hpp>

namespace api::auth
{
  void register_routes(vix::App &app);
}

#endif
```

Implementation:

```cpp
// modules/auth/src/auth.cpp
#include <auth/api.hpp>

namespace api::auth
{
  void register_routes(vix::App &app)
  {
    app.get("/auth/health", [](vix::Request &, vix::Response &res) {
      res.json({
        "ok", true,
        "module", "auth"
      });
    });
  }
}
```

App:

```cpp
#include <vix.hpp>
#include <auth/api.hpp>

int main()
{
  vix::App app;

  api::auth::register_routes(app);

  app.run();
  return 0;
}
```

## Module service pattern

A module can expose application services.

Example:

```cpp
// modules/users/include/users/api.hpp
#ifndef api_users_api_hpp
#define api_users_api_hpp

#include <string>

namespace api::users
{
  struct UserService
  {
    std::string find_name_by_id(int id) const;
  };
}

#endif
```

Implementation:

```cpp
// modules/users/src/users.cpp
#include <users/api.hpp>

namespace api::users
{
  std::string UserService::find_name_by_id(int id) const
  {
    return "user-" + std::to_string(id);
  }
}
```

## Modules and registry dependencies

Modules can use registry dependencies, but registry dependencies belong to the app dependency model.

Example:

```ini
deps = [
  gk/json@^1.0.0,
]

links = [
  vix::vix,
  gk::json,
]
```

Then a module can link to the dependency in its own `CMakeLists.txt` if needed.

Example:

```cmake
target_link_libraries(api_users PRIVATE gk::json)
```

Keep the rule:

```txt
vix.app declares app-level packages and registry deps
module CMake declares module-level target dependencies
```

## When to move to manual CMake

Stay with `vix.app` when your app has:

```txt
one main target
simple modules
normal packages
normal links
normal resources
```

Move to `CMakeLists.txt` when you need:

```txt
multiple custom targets
custom commands
generated sources
install rules
CTest integration
complex platform-specific logic
advanced package exports
custom toolchains
deep CMake functions
```

Adding a `CMakeLists.txt` is enough.

Vix will automatically use CMake because CMake has priority.

## Commands

| Command                  | Purpose                    |
| ------------------------ | -------------------------- |
| `vix modules init`       | Initialize module support. |
| `vix modules add <name>` | Create a new module.       |
| `vix modules check`      | Validate module rules.     |
| `vix modules --help`     | Show help.                 |

## Options

| Option             | Description                                         |
| ------------------ | --------------------------------------------------- |
| `-d, --dir <path>` | Project directory.                                  |
| `--dir=<path>`     | Same as `--dir <path>`.                             |
| `--project <name>` | Project name used for generated module targets.     |
| `--project=<name>` | Same as `--project <name>`.                         |
| `--no-patch`       | Do not patch root `CMakeLists.txt` during `init`.   |
| `--patch`          | Allow root `CMakeLists.txt` patching during `init`. |
| `--no-link`        | Do not patch root CMake link block during `add`.    |
| `--link`           | Allow root CMake link patching during `add`.        |
| `-h, --help`       | Show help.                                          |

## Common workflows

### Start modules in a vix.app project

```bash
vix modules init
vix modules add auth
vix modules add users
```

Then edit `vix.app`:

```ini
modules = [
  auth,
  users,
]
```

Build:

```bash
vix build
```

### Start modules in a CMake project

```bash
vix modules init
vix modules add auth
vix build
```

If needed, link manually:

```cmake
target_link_libraries(api PRIVATE api::auth)
```

### Add a module without CMake auto-link patch

```bash
vix modules add auth --no-link
```

Then either add it to `vix.app`:

```ini
modules = [
  auth,
]
```

or link manually in CMake:

```cmake
target_link_libraries(api PRIVATE api::auth)
```

### Check module rules

```bash
vix modules check
```

### Run from another directory

```bash
vix modules add auth --dir ./apps/api
```

### Force project name

```bash
vix modules add auth --project api
```

## Common mistakes

### Forgetting to add the module to vix.app

Wrong:

```bash
vix modules add auth
vix build
```

without updating:

```ini
modules = [
  auth,
]
```

Correct:

```bash
vix modules add auth
```

Then:

```ini
modules = [
  auth,
]
```

### Editing generated CMake

Wrong:

```txt
.vix/generated/app/CMakeLists.txt
```

Correct:

```txt
vix.app
```

For simple apps, edit `vix.app`.

For advanced apps, create a real `CMakeLists.txt`.

### Using modules for registry packages

Wrong:

```ini
modules = [
  gk/json@^1.0.0,
]
```

Correct:

```ini
deps = [
  gk/json@^1.0.0,
]
```

### Forgetting cross-module CMake dependencies

Wrong:

```cpp
#include <auth/api.hpp>
```

inside `users`, but no link in `modules/users/CMakeLists.txt`.

Correct:

```cmake
target_link_libraries(api_users PUBLIC api::auth)
```

or:

```cmake
target_link_libraries(api_users PRIVATE api::auth)
```

### Using relative includes

Wrong:

```cpp
#include "../modules/auth/include/auth/api.hpp"
```

Correct:

```cpp
#include <auth/api.hpp>
```

### Creating generic dumping-ground modules

Avoid:

```txt
utils
helpers
common
misc
```

Prefer domain modules:

```txt
auth
users
orders
billing
notifications
```

### Expecting vix.app to be used when CMakeLists.txt exists

If your project has:

```txt
CMakeLists.txt
vix.app
```

Vix uses:

```txt
CMakeLists.txt
```

If you want to test `vix.app`, temporarily rename `CMakeLists.txt`.

## Troubleshooting

### `modules/ folder not found`

Run:

```bash
vix modules init
```

Then:

```bash
vix modules add auth
```

### `Module already exists`

The folder already exists:

```txt
modules/auth
```

Choose another name or remove the existing module intentionally.

### `Invalid module name`

Use only:

```txt
letters
numbers
_
-
```

Good:

```bash
vix modules add auth
vix modules add user_profiles
vix modules add payment-gateway
```

Bad:

```bash
vix modules add "user profiles"
vix modules add auth/core
```

### `Reserved module name`

Choose a domain name instead of a tool, folder, or framework name.

Good:

```txt
billing
orders
files
notifications
```

Bad:

```txt
src
include
vix
core
sqlite
boost
```

### Module compiles but app cannot include header

Make sure the module is declared in `vix.app`:

```ini
modules = [
  auth,
]
```

For CMake projects, make sure the app links the module:

```cmake
target_link_libraries(api PRIVATE api::auth)
```

### Module includes another module but build fails

Add the module dependency:

```cmake
target_link_libraries(api_users PRIVATE api::auth)
```

Use `PUBLIC` if the dependency appears in public headers.

### vix.app project says CMake patch skipped

That is expected.

For `vix.app`, add modules in:

```ini
modules = [
  auth,
]
```

Root CMake patching is only for CMake projects.

## Best practices

Use `vix.app` for simple application modules.

Use `modules = [...]` as the source of truth in app-first projects.

Use CMake only when you need advanced build control.

Keep module names domain-focused.

Expose public headers under:

```txt
modules/<name>/include/<name>/
```

Keep private implementation under:

```txt
modules/<name>/src/
```

Use angle-bracket includes:

```cpp
#include <auth/api.hpp>
```

Declare cross-module dependencies explicitly with `target_link_libraries`.

Use `PUBLIC` only when the dependency appears in public headers.

Use `PRIVATE` when the dependency is only used in implementation files.

Run:

```bash
vix modules check
```

before committing larger module changes.

Do not edit generated CMake under `.vix/generated/app`.

## Related commands

| Command       | Purpose                                 |
| ------------- | --------------------------------------- |
| `vix build`   | Build the app and its modules.          |
| `vix run`     | Run the modular app.                    |
| `vix dev`     | Rebuild and restart during development. |
| `vix check`   | Validate project health.                |
| `vix tests`   | Run tests.                              |
| `vix add`     | Add registry dependencies.              |
| `vix install` | Install project dependencies.           |

## Next step

Learn how `vix.app` resolves projects and falls back to CMake.

[Open the CMake fallback guide](/guides/vix-app/cmake-fallback)
