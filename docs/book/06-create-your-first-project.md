# Create your first project

In the previous chapter, you ran a single C++ file. Now you will create a real Vix project.

A project gives you: source files, tests, CMake configuration, Vix manifest, tasks, environment files, development workflow, and release workflow.

```bash
vix new app
```

## The goal

Understand: how to create a Vix project, what files Vix generates, how to run it, how to use tasks, how `.env` works, and how `vix.json` works.

## Create a project

```bash
vix new app
cd app
```

For your first project, choose the **Application** template.

## Generated files

```txt
app/
├── app.vix
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── .env
├── .env.example
├── src/
│   └── main.cpp
├── tests/
│   └── test_basic.cpp
└── vix.json
```

| File/Folder | Purpose |
|-------------|---------|
| `src/` | Application source code |
| `tests/` | Project tests |
| `.env` | Local runtime configuration |
| `app.vix` | Vix project manifest for running |
| `vix.json` | Package and task manifest |
| `CMakeLists.txt` | C++ build configuration |
| `CMakePresets.json` | Build presets |

## `src/main.cpp`

The generated entrypoint:

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.send("Hello world");
  });

  app.run(8080);
}
```

## Run the project

```bash
vix run          # build and run
vix dev          # development mode (hot reload)
vix build        # compile only
vix tests        # run tests
vix check --tests  # validate project
vix fmt          # format source files
```

## Project tasks

```bash
vix task dev
vix task test
vix task check
vix task ci
vix task release
```

## `vix.json`

```json
{
  "name": "app",
  "deps": [],
  "vars": {
    "preset": "dev-ninja",
    "release_preset": "release",
    "log_level": "info"
  },
  "tasks": {
    "fmt": "vix fmt",
    "dev": {
      "description": "Start dev mode",
      "command": "vix dev"
    },
    "test": {
      "description": "Run tests",
      "command": "vix tests --preset ${preset} --fail-fast"
    },
    "check": {
      "description": "Validate project",
      "command": "vix check --preset ${preset} --tests"
    }
  }
}
```

## Dependencies

```bash
vix add namespace/package   # add a dependency
vix install                 # install from vix.lock
```

`vix.json` → declared dependencies,
`vix.lock` → exact resolved versions,
`.vix/deps` → installed packages.

## `app.vix`

```toml
.vix
version = 1

[app]
kind = "project"
dir = "."
name = "app"
entry = "src/main.cpp"

[build]
preset = "dev-ninja"
run_preset = "run-dev-ninja"
```

run it:

```bash
vix dev api.vix
# or
vix run api.vix
```

## `.env`

```dotenv
SERVER_PORT=8080
DATABASE_ENGINE=mysql
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_PASSWORD=
DATABASE_DEFAULT_NAME=appdb
```

## Using config in code

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  config::Config cfg{".env"};

  App app;

  app.get("/", [](Request &, Response &res){
    res.send("Hello world");
  });

  app.run(cfg.getServerPort());
}
```

## Recommended first edit

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  config::Config cfg{".env"};
  App app;

  app.get("/", [](Request &, Response &res){
    res.json({
      "message", "Hello from your first Vix project",
      "framework", "Vix.cpp"
    });
  });

  app.get("/health", [](Request &, Response &res){
    res.json({
      "ok", true,
      "service", "first-vix-project"
    });
  });

  app.run(cfg.getServerPort());

  return 0;
}
```

```bash
vix dev
curl -i http://127.0.0.1:8080/health
```

## Daily workflow

```bash
# Development
vix dev

# Before committing
vix task fmt
vix task check
vix task test
# or
vix task ci
```

## Common mistakes

### Running from the wrong folder

```bash
# Wrong
vix dev    # from outside the project

# Correct
cd app
vix dev
```

### Forgetting `.env`

```bash
cp .env.example .env
```

### Adding source files without updating CMake

If you create `src/routes/PublicRoutes.cpp`, add it to `CMakeLists.txt`:

```cmake
add_executable(app
  src/main.cpp
  src/routes/PublicRoutes.cpp
)
```

## What you should remember

A Vix project gives you a real structure with
`src/`,
`tests/`,
`.env`,
`app.vix`,
`vix.json`,
`CMakeLists.txt`.

The core idea: a Vix project is the point where quick experiments become a real application.

## Next chapter

[Next: First HTTP server](/book/07-first-http-server)
