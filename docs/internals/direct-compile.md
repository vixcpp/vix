# Direct Compile

Direct compile is the fast path behind `vix run main.cpp`.
It allows a single C++ file to be compiled and run immediately.

```txt
main.cpp → detect script mode → compile directly → cache binary → run
```

## The problem direct compile solves

Traditional C++ requires build setup even for small files.
Direct compile removes this friction:

```bash
mkdir -p ~/tmp/vix-demo
cd ~/tmp/vix-demo
touch main.cpp
vix run main.cpp
```

## The basic flow

```txt
read CLI arguments → detect that main.cpp is a file → enter script mode
→ inspect options → build compiler command → compile to cache location
→ run produced binary → forward runtime args if any
```

## Script mode vs project mode

| Mode           | Best for                   | Build strategy                         |
|----------------|----------------------------|----------------------------------------|
| Direct compile | A single `.cpp` file.      | Compiles the file directly.            |
| Project build  | A real application project.| Uses project config and build system.  |

Direct compile answers: "Can I quickly compile and run this one file?"

## Compiler flags vs runtime arguments

This is the most important rule:

```bash
--    = compiler or linker flags
--run = runtime arguments passed to the program
```

```bash
# Compiler flags
vix run main.cpp -- -O2 -DNDEBUG
vix run main.cpp -- -lssl -lcrypto

# Runtime arguments
vix run main.cpp --run server
vix run main.cpp --run client 127.0.0.1 9101

# Wrong — sends "server" to the compiler
vix run main.cpp -- server
```

```cpp
// Inside your program, --run args appear in argv
int main(int argc, char **argv)
{
  // argv[1] == "server" when: vix run main.cpp --run server
}
```

## Feature flags

```bash
vix run main.cpp --with-sqlite   # SQLite support
vix run main.cpp --with-mysql    # MySQL support
vix run main.cpp --san           # sanitizers
vix run main.cpp --ubsan         # UBSan only
vix run main.cpp --watch         # watch and rerun on change
vix run main.cpp --verbose       # verbose output
vix run main.cpp --quiet         # less output
vix run main.cpp --no-clear      # preserve terminal output
vix run main.cpp --local-cache   # local script cache behavior
```

## Cache model

```txt
source file + compiler flags + feature flags + Vix version
  ↓
cache key
  ↓
compiled binary (reused if inputs unchanged)
```

```txt
first run:  compile → save artifact → run
next run:   check cache → reuse if valid → run
```

## Include resolution

```bash
# Check SDK is installed
find ~/.local/include -name vix.hpp 2>/dev/null
# Expected: ~/.local/include/vix.hpp
```

If missing: install the full SDK, not CLI-only mode.

## Public headers in docs examples

```cpp
#include <vix.hpp>              // most examples
#include <vix/json.hpp>
#include <vix/db.hpp>
#include <vix/cache.hpp>
#include <vix/sync.hpp>
#include <vix/p2p.hpp>
#include <vix/websocket.hpp>
#include <vix/middleware.hpp>
```

Docs examples should use public headers, not internal repository paths.

## Working directory

The program runs from the current working directory.
Relative paths depend on where you run the command:

```cpp
res.file("public/index.html");  // looks for ./public/index.html
```

```bash
cd ~/tmp/vix-examples/my-app
vix run main.cpp  # correct
```

## Direct compile examples

```bash
# HTTP server
vix run main.cpp

# With database
vix run main.cpp --with-sqlite

# With runtime modes
vix run main.cpp --run server
vix run main.cpp --run client 127.0.0.1 9101

# With compiler optimization
vix run main.cpp -- -O2 -DNDEBUG
```

## Fallback conditions

Vix may fall back to project mode when:

- the app has multiple translation units
- custom CMake configuration is required
- the manifest explicitly describes a project
- the current directory is a known Vix project
- complex dependency resolution is required

Rule: **fast path when safe, project path when needed.**

## Direct compile lifecycle

```txt
start vix run → parse command → detect script mode → resolve flags
→ check cache → compile if needed → run binary → stream output → return exit code

(for long-running servers: compile → run → server stays alive → Ctrl+C → exit)
```

## Practical command reference

```bash
vix run main.cpp                        # run a file
vix run main.cpp --run server           # runtime arguments
vix run main.cpp -- -O2                 # compiler flags
vix run main.cpp --with-sqlite          # SQLite support
vix run main.cpp --with-mysql           # MySQL support
vix run main.cpp --san                  # sanitizers
vix run main.cpp --watch                # watch mode
vix run main.cpp --verbose              # verbose
vix run main.cpp --no-clear             # preserve terminal
```

## Common mistakes

```bash
# Wrong — program args after --
vix run main.cpp -- --port 8080
# Correct
vix run main.cpp --run --port 8080

# Wrong — forgetting feature flag
vix run main.cpp        # SQLite code won't compile
# Correct
vix run main.cpp --with-sqlite

# Wrong — wrong directory for relative paths
cd /tmp && vix run ~/my-app/main.cpp  # public/ not found
# Correct
cd ~/my-app && vix run main.cpp
```

## Design principles

1. **Fast simple path:** `vix run main.cpp` should just work.
2. **Keep C++ explicit:** Vix makes the workflow smoother, not invisible.
3. **Cache safely:** never reuse stale binaries incorrectly.
4. **Fall back when needed:** project mode exists for complex cases.
5. **Good diagnostics:** when compilation fails, explain what to try next.

## What you should remember

```bash
--    = compiler or linker flags
--run = runtime arguments

vix run main.cpp --run arg1 arg2   # runtime args
vix run main.cpp -- -O2            # compiler flags
```

The core idea: **direct compile makes C++ feel immediate for the simple case, while preserving the full project path for serious applications.**

Next: [Error Diagnostics](/internals/error-diagnostics)
