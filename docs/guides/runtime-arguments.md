# Runtime Arguments

Runtime arguments are arguments passed to the program started by Vix.
Use this guide when you want to pass values to your app at runtime without changing the source code.

## The idea

`vix run` has two kinds of arguments:

```txt
Vix arguments
Runtime arguments
```

Vix arguments control the Vix command itself.

Runtime arguments are passed to your program.

The separator is:

```bash
--
```

Everything after `--` belongs to the application.

## Basic usage

```bash
vix run main.cpp -- --name Gaspard
```

Here:

```txt
vix run main.cpp
```

is the Vix command.

And:

```txt
--name Gaspard
```

is passed to the running program.

## Why `--` matters

Without `--`, Vix treats the arguments as Vix options.

With `--`, Vix stops parsing and forwards the rest to your app.

```bash
vix run main.cpp -- --debug --port 8080
```

The app receives:

```txt
--debug --port 8080
```

## Single-file C++ example

Create `main.cpp`:

```cpp
#include <vix.hpp>

#include <iostream>

int main(int argc, char **argv)
{
  vix::print("Runtime arguments:");

  for (int i = 0; i < argc; ++i)
  {
    std::cout << "argv[" << i << "] = " << argv[i] << '\n';
  }

  return 0;
}
```

Run it:

```bash
vix run main.cpp -- --name Gaspard --mode dev
```

Expected shape:

```txt
Runtime arguments:
argv[0] = ...
argv[1] = --name
argv[2] = Gaspard
argv[3] = --mode
argv[4] = dev
```

## Project example

Inside a Vix project:

```bash
vix run -- --port 8080
```

Or with an explicit target:

```bash
vix run api -- --port 8080 --debug
```

The runtime arguments are forwarded to the executable that Vix starts.

## Common examples

```bash
# Pass a port
vix run main.cpp -- --port 8080

# Pass a mode
vix run main.cpp -- --mode dev

# Pass several values
vix run main.cpp -- --host 127.0.0.1 --port 8080

# Pass positional arguments
vix run main.cpp -- input.txt output.txt

# Pass flags to a project app
vix run -- --debug --seed 42
```

## Runtime arguments with `vix dev`

`vix dev` is for active development.

When supported by the project workflow, the same separator idea applies:

```bash
vix dev -- --debug --port 8080
```

Use this when you want the dev process to restart with the same runtime arguments.

## Runtime arguments with replay

When recording a run:

```bash
vix run main.cpp --replay -- --name Gaspard
```

Later:

```bash
vix replay last
```

Vix replays the stored command and its recorded runtime arguments.

You can also append extra arguments during replay:

```bash
vix replay last -- --extra value
```

## Runtime arguments vs environment variables

Use runtime arguments for values that belong to one execution.

Use environment variables for configuration that should be loaded from the environment.

Runtime argument:

```bash
vix run main.cpp -- --seed 42
```

Environment variable:

```bash
VIX_LOG_LEVEL=debug vix run main.cpp
```

`.env` configuration:

```dotenv
SERVER_PORT=8080
VIX_LOG_LEVEL=info
```

A good rule:

```txt
runtime arguments = this run
environment variables = app configuration
```

## Runtime arguments vs compile flags

Runtime arguments go to the running program.

Compile flags go to the compiler.

Runtime arguments:

```bash
vix run main.cpp -- --debug
```

Compile or build flags are not runtime arguments.

Do not mix them.

## Recommended pattern for apps

For application configuration, prefer `.env`:

```dotenv
SERVER_PORT=8080
APP_ENV=local
```

For one-time execution values, use runtime arguments:

```bash
vix run api -- --import users.csv
```

This keeps the source code stable and the command explicit.

## Common mistakes

### Forgetting `--`

Wrong:

```bash
vix run main.cpp --name Gaspard
```

Correct:

```bash
vix run main.cpp -- --name Gaspard
```

### Passing app flags before the separator

Wrong:

```bash
vix run main.cpp --debug -- --port 8080
```

Correct:

```bash
vix run main.cpp -- --debug --port 8080
```

### Using runtime arguments for stable configuration

For stable app settings, prefer `.env`:

```dotenv
SERVER_PORT=8080
```

Then run:

```bash
vix run
```

### Expecting runtime arguments to change build behavior

Runtime arguments affect the program after it starts.

They do not change compilation, dependency resolution, or build presets.

## Practical workflow

```bash
# Run normally
vix run

# Run with app arguments
vix run -- --debug --port 8080

# Run a C++ file with app arguments
vix run main.cpp -- --name Gaspard

# Record and replay the same runtime arguments
vix run main.cpp --replay -- --name Gaspard
vix replay last
```

## Related commands

| Command      | Purpose                                                                 |
| ------------ | ----------------------------------------------------------------------- |
| `vix run`    | Build and run a file, executable, manifest, Docker workflow, or project |
| `vix dev`    | Run development mode                                                    |
| `vix replay` | Replay a recorded execution                                             |
| `vix env`    | Check project environment files                                         |
| `vix build`  | Build without running                                                   |

## What you should remember

Use `--` to separate Vix arguments from application arguments.

```bash
vix run main.cpp -- --name Gaspard
```

Everything after `--` is passed to your app.

Use `.env` for stable configuration.

Use runtime arguments for values specific to one execution.

## Next step

Continue with diagnostics.

[Open the diagnostics guide](/guides/diagnostics)
