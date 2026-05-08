# vix dev

`vix dev` starts a Vix application in development mode.

It configures, builds, runs, watches, rebuilds, and restarts your app automatically while you edit code.

Use it during active development.

```bash
vix dev
```

## Overview

`vix dev` is the development entrypoint for Vix projects and single-file C++ apps.

It is designed for a fast feedback loop:

1. edit file
2. save
3. Vix detects the change
4. Vix rebuilds the target
5. Vix restarts the app

In project mode, `vix dev` uses the project target by default, not the full `all` target.

This keeps development reloads focused and fast.

## Usage

```bash
vix dev [name] [options] [-- app-args...]
```

## Basic usage

```bash
# Start the current project in dev mode
vix dev

# Start a named app or target
vix dev api

# Start a single C++ file in dev mode
vix dev server.cpp

# Pass runtime arguments to the application
vix dev server.cpp -- --port 8080
```

## What vix dev does

When you run:

```bash
vix dev
```

Vix performs these steps:

1. Detect the current project
2. Configure the project if needed
3. Build the main target
4. Start the application
5. Watch relevant files
6. Rebuild when files change
7. Restart the application automatically

Normal output is intentionally compact:

```
Dev api (dev)
  ✔ Configured
Compiling api (dev)
  build [============================] done
  ✔ Started pid=12345
```

When the application starts, the app may print its own output:

```
2:36:00 AM  ◆ Vix.cpp   READY   v2.5.3 (1 ms)   dev

  › HTTP:    http://localhost:8080/
  i Threads: 8/8
  i Mode:    dev (watch/reload)
  i Status:  ready
  i Hint:    Ctrl+C to stop the server
```

## Development profile

The `(dev)` label means Vix is running in development profile.

| Label | Description |
|-------|-------------|
| `(dev)` | development build, debug-friendly, fast feedback |
| `(release)` | optimized build, production-oriented |

For `vix dev`, the public profile is always development-oriented.

Example:

```
Dev api (dev)
Compiling api (dev)
```

## Project mode

Inside a Vix project:

```bash
vix dev
```

Vix uses the project directory name as the default target.

For example, `~/tmp/api` uses target `api`. So `vix dev` starts the `api` target.

You can also pass a target or app name explicitly:

```bash
vix dev api
```

## Script mode

You can run a single `.cpp` file in development mode:

```bash
vix dev server.cpp
```

This is useful for quick prototypes, small servers, experiments, or demos without creating a full project.

Example:

```bash
vix dev server.cpp --force-server
```

## Runtime arguments

Arguments after `--` are passed to your application.

```bash
vix dev server.cpp -- --port 8080
```

For project mode:

```bash
vix dev -- --port 8080
```

The `--` separator matters because Vix needs to distinguish its own options from your app arguments.

### Important argument rule

In script mode, `vix dev server.cpp -- --port 8080` means `--port 8080` goes to your application.

Without `--`, Vix may treat the argument as a Vix option.

Wrong:

```bash
vix dev server.cpp --port 8080
```

Correct:

```bash
vix dev server.cpp -- --port 8080
```

## Watch and reload

Watch mode is enabled by default in `vix dev`.

These are equivalent:

```bash
vix dev
vix dev --watch
vix dev --reload
```

When a watched file changes, Vix clears the terminal, rebuilds, and restarts the application.

Example reload output:

```
Dev api (dev)
  changed: /home/user/api/src/main.cpp
Compiling api (dev)
  build [============================] done
  ✔ Started pid=12389
```

## File watching behavior

`vix dev` watches files that can affect the build or runtime.

Watched source extensions: `.cpp`, `.cc`, `.cxx`, `.c`, `.hpp`, `.hh`, `.hxx`, `.h`, `.ipp`, `.inl`, `.cmake`

Watched project files: `CMakeLists.txt`, `CMakePresets.json`, `vix.json`, `vix.toml`, `vix.lock`

Ignored paths: `.git`, `.vix`, `build`, `build-dev`, `build-ninja`, `build-release`, `node_modules`, `.cache`, `.idea`, `.vscode`, `docs`, `doc`, `dist`, `out`, `coverage`

Ignored files include common non-build files such as `README.md`, `CHANGELOG.md`, `LICENSE`, `.gitignore`.

## Rebuild behavior

`vix dev` uses a file index to detect changes.
It compares mtime, file size, and change kind. This avoids unnecessary rebuilds and makes reload detection faster.

### Source or header change

When a `.cpp`, `.hpp`, or `.h` file changes, Vix performs a normal rebuild and restarts the app.

```
Dev api (dev)
  changed: /home/user/api/src/main.cpp
Compiling api (dev)
  build [============================] done
  ✔ Started pid=12389
```

### CMake or Vix config change

When a configuration file changes (`CMakeLists.txt`, `CMakePresets.json`, `vix.json`, `vix.lock`), Vix performs a reconfigure, rebuild, and restart.
This is needed because the project structure, dependencies, options, or targets may have changed.

## Build progress

`vix dev` uses the same build progress style as `vix build`.

When Ninja reports progress, Vix renders it as:

```
Compiling api (dev)
  build [====------------------------] 4/31
  › Building CXX object CMakeFiles/api.dir/src/main.cpp.o
```

When the build is complete:

```
  build [============================] done
```

The progress is based on real Ninja build progress, not a fake spinner.

### Why progress may not appear instantly

Sometimes `Compiling api (dev)` appears before the progress line because Vix is waiting for Ninja to emit the first real progress line. This is normal.
Vix does not show fake progress in `vix dev`.

## Configure behavior

If the build directory does not exist yet, Vix configures the project first:

```
Dev api (dev)
  ✔ Configured
Compiling api (dev)
  build [============================] done
  ✔ Started pid=12345
```

If the project is already configured, Vix skips the configure step.

## Build directory

Project dev mode uses `build-ninja`.

Example:

```
/home/user/api/build-ninja
```

This matches the normal development build path used by `vix build`.

## Target behavior

By default, `vix dev` builds the main project target. For a project named `api`, Vix builds `api`, not `all`. This avoids rebuilding examples, tests, and auxiliary targets during development reloads.

## Force server mode

Use `--force-server` when the target is a long-running app such as an HTTP server or WebSocket server.

```bash
vix dev server.cpp --force-server
```

## Force script mode

Use `--force-script` when the target is a short-lived CLI tool.

```bash
vix dev tool.cpp --force-script
```

## Parallel build jobs

Use `-j` or `--jobs` to control parallel build jobs.

```bash
vix dev -j 8
vix dev --jobs 16
```

If no job count is provided, Vix uses a sensible default based on the machine.

## SQLite support

Use `--with-sqlite` when your app needs SQLite-related Vix database support.

```bash
vix dev --with-sqlite
vix dev server.cpp --with-sqlite
```

## MySQL support

Use `--with-mysql` when your app needs MySQL-related Vix database support.

```bash
vix dev --with-mysql
vix dev server.cpp --with-mysql
```

When enabled, Vix can also expose runtime environment variables such as:

```
VIX_DB_ENGINE=mysql
VIX_ENABLE_DB=1
VIX_DB_USE_MYSQL=1
```

or:

```
VIX_DB_ENGINE=sqlite
VIX_ENABLE_DB=1
VIX_DB_USE_SQLITE=1
```

## Sanitizers

`vix dev` supports sanitizer-related options inherited from `vix run`.

```bash
vix dev server.cpp --san
vix dev server.cpp --ubsan
vix dev server.cpp --tsan
```

Use these when debugging memory issues, undefined behavior, or threading problems.

## Logging

Use `--log-level` to control Vix logging.

```bash
vix dev --log-level debug
```

Supported levels: `trace`, `debug`, `info`, `warn`, `error`, `critical`

Use `--verbose` for debug-style output:

```bash
vix dev --verbose
```

Use `--quiet` for minimal output:

```bash
vix dev --quiet
```

## Verbose mode

Normal mode keeps output small:

```
Dev api (dev)
  ✔ Configured
Compiling api (dev)
  build [============================] done
  ✔ Started pid=12345
```

Verbose mode shows more detail:

```bash
vix dev -v
```

```
Dev api (dev)
  watching: /home/user/api
  target  : api
  build   : /home/user/api/build-ninja
  press Ctrl+C to stop

Configuring api (dev)
  ✔ Configured

Compiling api (dev)
  build [============================] done
  ✔ Started pid=12345
```

## Quiet mode

```bash
vix dev --quiet
vix dev -q
```

## Clean rebuild

Use `vix build --clean` when you want to clean the build directory before running dev mode again.

```bash
vix build --clean
vix dev
```

For most normal development, you do not need to clean manually.

## Recommended workflow

### New project

```bash
vix new api
cd api
vix install
vix dev
```

Then edit your files. When you save:

```
Dev api (dev)
  changed: /home/user/api/src/main.cpp
Compiling api (dev)
  build [============================] done
  ✔ Started pid=12389
```

### Existing project

```bash
cd api
vix install
vix dev
```

### Single-file server

```bash
vix dev server.cpp --force-server
vix dev server.cpp --force-server -- --port 8080
```

### Single-file CLI

```bash
vix dev tool.cpp --force-script
vix dev tool.cpp --force-script -- input.txt
```

## Difference between vix dev, vix run, and vix build

| Command | Purpose | Runs app | Watches files | Restarts app |
|---------|---------|----------|---------------|--------------|
| `vix build` | Configure and compile | no | no | no |
| `vix run` | Build and run once | yes | no by default | no by default |
| `vix run --watch` | Build, run, and watch | yes | yes | yes |
| `vix dev` | Development mode | yes | yes | yes |

`vix dev` is the best command while editing an application. `vix run` is better when you want to run manually.
`vix build` is better when you only want to compile.

## Relationship with vix run --watch

Conceptually, `vix dev` is similar to `vix run --watch`, but `vix dev` is optimized for the development experience.
It provides cleaner dev output, target-aware rebuilds, automatic reload behavior, file indexing, and development-focused terminal rendering.

## Options

| Option | Description |
|--------|-------------|
| `--force-server` | Force classification as a long-running development server. |
| `--force-script` | Force classification as a short-lived script or CLI tool. |
| `--watch` | Enable watch mode. Enabled by default in `vix dev`. |
| `--reload` | Alias-style reload flag. Enabled by default in `vix dev`. |
| `-j, --jobs <n>` | Number of parallel build jobs. |
| `--with-sqlite` | Enable SQLite-related support. |
| `--with-mysql` | Enable MySQL-related support. |
| `--san` | Enable AddressSanitizer and UBSan where supported. |
| `--ubsan` | Enable UBSan-only mode where supported. |
| `--tsan` | Enable ThreadSanitizer where supported. |
| `--cwd <path>` | Run the application from a specific working directory. |
| `--env <KEY=VALUE>` | Pass an environment variable to the app. |
| `--args <value>` | Pass repeatable runtime arguments. |
| `--log-level <level>` | Set Vix log verbosity. |
| `--verbose, -v` | Show more development details. |
| `--quiet, -q` | Reduce output to warnings and errors. |
| `-h, --help` | Show command help. |

## Environment variables

| Variable | Description |
|----------|-------------|
| `VIX_LOG_LEVEL=debug` | Show debug-level Vix logs. |
| `VIX_LOG_LEVEL=trace` | Show trace-level Vix logs. |
| `VIX_MODE=dev` | Set automatically for the running app. |
| `VIX_STDOUT_MODE=line` | Set automatically to make runtime output easier to stream. |
| `VIX_DB_ENGINE=mysql` | Set when MySQL mode is enabled. |
| `VIX_DB_ENGINE=sqlite` | Set when SQLite mode is enabled. |

## Common workflows

### Start a new app

```bash
vix new api
cd api
vix install
vix dev
```

### Run an existing app

```bash
cd api
vix install
vix dev
```

### Run a server file directly

```bash
vix dev server.cpp --force-server
```

### Run a CLI tool file directly

```bash
vix dev tool.cpp --force-script
```

### Pass app arguments

```bash
vix dev server.cpp -- --port 8080
```

### Use more build jobs

```bash
vix dev -j 8
```

### Enable verbose dev output

```bash
vix dev -v
```

### Enable debug logs

```bash
VIX_LOG_LEVEL=debug vix dev
```

## Common mistakes

### Running outside the project directory

Wrong:

```bash
vix new api
vix dev
```

Correct:

```bash
vix new api
cd api
vix dev
```

### Forgetting `--` before app arguments

Wrong:

```bash
vix dev server.cpp --port 8080
```

Correct:

```bash
vix dev server.cpp -- --port 8080
```

### Expecting vix dev to build every target

`vix dev` builds the main project target. It does not build every example, test, or auxiliary target by default.

```bash
vix build --build-target all
```

### Using VIX_LOG_LEVEL=release

Wrong:

```bash
VIX_LOG_LEVEL=release vix dev
```

Correct:

```bash
vix build --preset release
```

`VIX_LOG_LEVEL` controls logging, not the build profile.

### Editing ignored files and expecting reload

Files such as `README.md`, `docs`, build output, `.git`, and `.vix` are ignored by dev mode.
Editing them does not trigger a reload.

## Troubleshooting

### Nothing happens after saving

Check that the file is watched. Watched files include source, header, CMake, and Vix config files.
Files such as docs, README, generated output, and build directories are ignored.

### The app does not restart

Make sure the build succeeds. If the build fails, Vix waits for the next file change.
Fix the error, save again, and Vix will rebuild automatically.

### The executable cannot be found

Vix expects the project executable target to match the project target name.
For example, project directory `api` expects target `api`.
Make sure your `CMakeLists.txt` defines an executable target with that name.

### Progress appears late

Vix only shows the progress bar when Ninja reports real progress.
If the build is already up to date or Ninja has not emitted progress yet, you may see `Compiling api (dev)` before the progress line. This is normal.

### Too much output

```bash
vix dev --quiet
```

### Need more details

```bash
vix dev -v
VIX_LOG_LEVEL=debug vix dev
```

## Best practices

Use `vix dev` while coding.
Use `vix build` before committing or packaging.
Use `vix build --build-target all` before install or full repository validation.
Use `vix check` when you want deeper validation.

---

## Related commands

| Command | Purpose |
|---------|---------|
| `vix run` | Build and run manually |
| `vix run --watch` | Build, run, and reload manually |
| `vix build` | Configure and compile |
| `vix check` | Validate build, tests, runtime, and sanitizers |
| `vix tests` | Run tests |
| `vix replay` | Inspect and replay previous Vix executions |

## Next step

Continue with project builds.

[Open the vix build guide](/cli/build)



