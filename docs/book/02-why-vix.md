# Why Vix ?

Vix exists because building real applications in C++ should be faster, clearer, and more practical.

C++ gives you: performance, control, deterministic lifetimes, low-level access, strong types, native binaries, and systems-level power.
But for many developers, the experience of building applications in C++ feels heavier than it should.

## The problem

To build a simple backend, you may need to think about compiler setup, CMake configuration, dependency installation, include paths, linker flags, build directories, runtime arguments, server lifecycle, logging, JSON, HTTP routing, database configuration, tests, and deployment.

Each piece is manageable alone. The problem is that you must wire everything together before the application becomes useful.

## The first step should be simple

In many ecosystems, the first step is direct:

```bash
python app.py
node server.js
deno run server.ts
```

Vix gives C++ a similar starting point:

```bash
vix run main.cpp
```

The goal is not to copy these ecosystems.
The goal is to make C++ feel more direct for application development.

## C++ does not lack power

The problem is not the language itself.
The problem is the missing application workflow around the language.
C++ already has great compilers, excellent performance, RAII, templates, zero-cost abstractions, native concurrency, mature tooling, and portable binaries.
What developers often miss is a unified runtime experience.

## The missing layer

When you build a backend in C++, you usually combine many pieces:
- HTTP library,
- JSON library,
- build system,
- logging library,
- database layer,
- middleware logic,
- validation layer,
- WebSocket library,
- and deployment scripts.

This creates friction.
Vix answers these questions with one coherent workflow.

## The Vix approach

```bash
vix run main.cpp          # Run a file
vix new api && vix dev    # Create and run a project
vix build                 # Build
vix check && vix tests    # Validate and test
```

## Why not just use CMake ?

Vix does not replace CMake.
CMake answers: "How do I configure and build this C++ project ?"

Vix answers: "How do I build, run, test, develop, package, and operate this C++ application ?"

Vix can use CMake underneath while giving the developer a simpler top-level experience.

## Why not just use a C++ web framework ?

A web framework usually focuses on HTTP.
Vix is broader.

Vix also includes the surrounding application workflow:
- CLI,
- project creation,
- direct file execution,
- build commands,
- dependency workflow,
- tests,
- formatting,
- logging,
- database access,
- WebSocket runtime,
- middleware,
- validation,
- cache,
- sync,
- P2P,
- and deployment path.

## The application comes first

Traditional C++ project setup often begins with build configuration. Vix wants the first step to be: write the application, run it.

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.text("Hello Vix");
  });

  app.run(8080);
  return 0;
}
```

```bash
vix run main.cpp
```

## Vix is explicit

A route is explicit:

```cpp
app.get("/users/{id}", [](Request &req, Response &res){
  const std::string id = req.param("id");
  res.json({"id", id});
});
```

A database query is explicit:

```cpp
auto stmt = conn->prepare("SELECT id, name FROM users WHERE id = ?");
stmt->bind(1, id);
auto rows = stmt->query();
```

This matters because serious systems must be understandable. Vix should reduce friction without hiding behavior.

## Why reliability matters

Vix is also connected to a bigger idea:
applications should keep working under real-world conditions.

Real systems face network failure, process restart, slow servers, partial writes, timeouts, offline clients, retries, duplicate requests, and lost connections.

This is why Vix includes deeper runtime ideas like cache, offline-first sync, WAL, outbox, retry, and P2P.

## What you should remember

Vix exists because C++ deserves a smoother application runtime.
The core problem is not that C++ is weak.
The problem is that building applications in C++ often starts with too much friction.

Vix provides:
- direct execution,
- project workflow,
- HTTP app model,
- JSON support,
- middleware,
- validation,
- database access,
- WebSocket runtime,
- production deployment path,
- and advanced reliability modules.

The core idea:
keep C++ powerful, make building applications with it feel direct.

## Next chapter

[Next: Mental model](/book/03-mental-model)
