# Introduction

Welcome to the Vix book.

This book teaches Vix step by step, like a story.
You will start with the simplest idea:

```txt
Run C++ code quickly.
```

Then you will grow toward real backend systems:
- HTTP APIs,
- JSON,
- middleware,
- validation,
- database,
- WebSocket,
- async runtime,
- cache,
- offline-first sync,
- P2P,
- and production deployment.

The goal is not only to show commands or APIs. The goal is to help you understand the mental model behind Vix.

## What is Vix ?

Vix is a modern C++ runtime for building fast and reliable applications. It gives C++ a more direct development experience:

```bash
vix run main.cpp
```

or:

```bash
vix new api
cd api
vix dev
```

Instead of forcing you to manually configure everything before writing your first line of useful code, Vix gives you a runtime-oriented workflow. You write the app. Vix handles the development loop.

## The big idea

C++ is powerful.
But building real applications in C++ often requires too much setup before the developer can even begin. You may need to think about CMake, compiler flags, linker flags, dependencies, build directories, runtime arguments, logs, tests, server startup, database flags, and project structure.

Vix tries to make this smoother. The idea is simple:

```txt
C++ should be able to feel direct without losing its power.
```

Vix does not remove C++. Vix gives C++ a better application runtime around it.

## Why a runtime ?

A language alone is not enough to build modern applications comfortably.

JavaScript became widely used for backend development not only because of the language, but because Node.js gave developers a runtime, package workflow, and fast feedback loop.

Python became popular for scripting and backend work because running a file is simple:

```bash
python app.py
node server.js
```

Vix brings that kind of workflow to C++:

```bash
vix run main.cpp
```

But with the performance, control, and explicitness of C++.

## What Vix is not

Vix is not a replacement for C++.
It is not a garbage-collected language.
It is not trying to hide how systems work or turn C++ into JavaScript or Python.

Vix is a runtime and toolkit that makes C++ application development more practical.
It keeps the C++ philosophy — explicit, fast, deterministic, close to the system — but adds a smoother workflow.

## A tiny example

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.json({
      "message", "Hello from Vix",
      "framework", "Vix.cpp"
    });
  });

  app.run(8080);
  return 0;
}
```

Run it:

```bash
vix run main.cpp
```

Then open `http://localhost:8080`.

This small example already shows the core Vix style:
- `App`,
- route,
- `Request`,
- `Response`,
- JSON,
- runtime.

## The development experience

```bash
vix new api    # Create a project
vix dev        # Run in development mode
vix build      # Build it
vix check      # Validate it
vix tests      # Run tests
vix fmt        # Format code
```

## The structure of this book

**Part 1: Understand Vix**
Introduction, Why Vix, Mental model

**Part 2: Start building**
Installation, Run your first file, Create your first project, First HTTP server

**Part 3: Build APIs**
Routes, Request and Response, JSON API

**Part 4: Add professional layers**
Middleware, Validation, Errors and logging

**Part 5: Connect real systems**
Database, Real-time WebSocket, Async runtime

**Part 6: Advanced runtime features**
Cache, Offline-first sync, P2P

**Part 7: Production**
Production deployment, Next steps

## How to read this book

Read it in order the first time.
Each chapter builds on the previous one.

You only need basic C++ knowledge:
- functions,
- classes,
- headers,
- `std::string`,
- `std::vector`,
- lambdas,
- and basic CMake awareness.

## The main mental shift

Traditional C++ development often starts with the build system.
Vix starts with the application.

Instead of thinking first about how to configure, link, and build, Vix wants the first question to be:

```txt
What do I want to build?
```

## Vix and production

A Vix app can run as a normal Linux service:

```txt
browser → Nginx → Vix app → systemd
```

The production chapter will show how to deploy with a release build, systemd, Nginx, TLS, logs, and health checks.

## What you should remember

Vix is a modern C++ runtime for building fast and reliable applications. It gives C++ a direct run workflow, a project workflow, an HTTP application model, JSON APIs, middleware, validation, database access, WebSocket support, runtime-oriented tools, and a production deployment path.

The core idea: keep the power of C++, make the application workflow simpler.

## Next chapter

[Next: Why Vix](/book/02-why-vix)
