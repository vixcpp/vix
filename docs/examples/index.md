# Examples

This section shows practical Vix examples.

```txt
learn concept → copy example → run it → modify it
```

The book teaches the story. The API reference explains the API surface. The examples section shows complete, focused applications you can copy, run, modify, and extend.

## How to use these examples

```bash
mkdir -p ~/tmp/vix-examples
cd ~/tmp/vix-examples
touch main.cpp
vix run main.cpp
vix run main.cpp --with-sqlite   # SQLite examples
vix run main.cpp --with-mysql    # MySQL examples
```

## Recommended order

| Order | Example | Purpose |
|-------|---------|---------|
| 1 | [Hello HTTP](/examples/hello-http) | Build the simplest HTTP server |
| 2 | [JSON API](/examples/json-api) | Return structured JSON responses |
| 3 | [Middleware](/examples/middleware) | Add logic around routes |
| 4 | [Authentication](/examples/auth) | Protect routes |
| 5 | [Database](/examples/database) | Store and read persistent data |
| 6 | [WebSocket](/examples/websocket) | Add realtime communication |
| 7 | [Cache](/examples/cache) | Add fast reads and resilience |
| 8 | [Sync](/examples/sync) | Make operations durable and retryable |
| 9 | [P2P](/examples/p2p) | Connect distributed nodes |
| 10 | [Production App](/examples/production-app) | Structure a deployable application |

## Common vix run rule

```bash
# Correct — --run passes args to your program
vix run main.cpp --run server
vix run main.cpp --run client 127.0.0.1 9101

# Wrong — -- is for compiler/linker flags
vix run main.cpp -- server
```

## Build flags

```bash
vix run main.cpp --with-sqlite      # SQLite support
vix run main.cpp --with-mysql       # MySQL support
vix run main.cpp -- -O2 -DNDEBUG    # compiler flags
vix run main.cpp --run arg1 arg2    # runtime arguments
```

## Before running examples

```bash
vix --version
find ~/.local/include -name vix.hpp 2>/dev/null
# Expected: ~/.local/include/vix.hpp
```

## Suggested workspace

```bash
mkdir -p ~/tmp/vix-examples
cd ~/tmp/vix-examples
mkdir hello-http json-api middleware database
```

## From examples to projects

A single-file example is good for learning. When the app grows:

```bash
vix new api
cd api
vix dev
```

## What you should remember

Start small. Run the code. Change one thing. Run it again.

```txt
Hello HTTP → JSON API → Middleware → Authentication → Database
→ WebSocket → Cache → Sync → P2P → Production App
```

Next: [Hello HTTP](/examples/hello-http)
