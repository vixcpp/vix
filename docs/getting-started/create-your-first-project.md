# Create Your First Project

This page shows how to create your first Vix.cpp project.

A single C++ file is useful for experiments:

```bash
vix run main.cpp
```

A project is the next step when the code needs a stable structure, configuration, tests, repeatable commands, and a path toward a real application.

For that, use:

```bash
vix new
```

A Vix.cpp project gives you a clean folder, an application manifest, configuration files, tests, metadata, and a standard command workflow.

## Create a simple application

Create a new application project:

```bash
cd ~/tmp
vix new hello --app
```

This creates a minimal runnable Vix application.

The CLI will print the next steps:

```txt
next
1  cd hello/    enter project
2  vix build    compile
3  vix run      start app
```

## Enter the project

Move into the generated folder:

```bash
cd hello
```

Run all project commands from inside this directory.

## Create local configuration

If the project contains `.env.example`, create your local `.env` file:

```bash
cp .env.example .env
```

This is the recommended workflow.

`.env.example` documents the configuration expected by the project.

`.env` contains the values used on your local machine.

This keeps environment-specific values outside the source code.

## Build the project

Compile the project:

```bash
vix build
```

Expected output shape:

```txt
Compiling hello (dev)
  ✔ Configured
  ✔ Built
  ✔ Done in 1.6s
```

`vix build` compiles the project without starting the application.

Use it when you want to verify that the project builds successfully.

## Run the project

Start the application:

```bash
vix run
```

Expected output shape:

```txt
● Vix.cpp   READY   v2.6.0   run
  › HTTP:    http://localhost:8080/
  i Threads: 8/8
  i Mode:    run
  i Status:  ready
  i Hint:    Ctrl+C to stop the server
```

Open another terminal and test the application:

```bash
curl http://127.0.0.1:8080/
```

Stop the server with:

```txt
Ctrl+C
```

## Development mode

For active development, use:

```bash
vix dev
```

Use `vix dev` when you are editing code and want a development-oriented workflow.

Use `vix run` when you simply want to start the application.

In practice:

```txt
vix build -> compile the project
vix run   -> start the application
vix dev   -> work on the application during development
```

## Generated project structure

A simple application project usually looks like this:

```txt
hello/
├── src/
│   └── main.cpp
├── tests/
│   └── test_basic.cpp
├── .env.example
├── .env
├── vix.app
├── vix.json
└── README.md
```

Some templates may include extra files depending on the selected project type.

## What each file does

| File or folder | Purpose                                                    |
| -------------- | ---------------------------------------------------------- |
| `src/main.cpp` | Main application entry point.                              |
| `tests/`       | Project tests.                                             |
| `.env.example` | Example configuration shared with the project.             |
| `.env`         | Local configuration for your machine.                      |
| `vix.app`      | Application manifest used by Vix.cpp to build the project. |
| `vix.json`     | Project metadata, tasks, and dependency information.       |
| `README.md`    | Generated project documentation.                           |

This structure is intentionally small. It gives the application enough organization to grow without forcing a complex layout too early.

## Open the entry file

Open:

```txt
src/main.cpp
```

A generated application keeps the entry point small.

The exact code may evolve between Vix.cpp versions, but the basic idea stays the same:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.send("Hello from Vix.cpp");
  });

  app.run();

  return 0;
}
```

The call to `app.run()` uses runtime configuration.

For normal applications, keep values such as the server port in `.env` instead of hardcoding them in the source code.

## Configuration

Open:

```txt
.env
```

You may see values such as:

```dotenv
SERVER_PORT=8080
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
```

To change the port, edit:

```dotenv
SERVER_PORT=3000
```

Then run again:

```bash
vix run
```

The source code does not need to change.

This pattern becomes important when the same application moves from local development to CI, staging, or production.

## `vix.app`

The `vix.app` file describes the application target.

It tells Vix.cpp what to build.

For simple projects, you do not need to write a `CMakeLists.txt` manually.

The flow is:

```txt
vix.app
  -> Vix.cpp generates the internal CMake project
  -> vix build compiles the app
  -> vix run starts the app
```

This gives the project a small readable manifest while keeping a real native C++ build underneath.

When the build itself becomes complex, the project can move to a normal `CMakeLists.txt`.

## `vix.json`

The `vix.json` file stores project metadata, tasks, and dependency information.

Some projects define tasks that you can run with:

```bash
vix task <name>
```

Common examples:

```bash
vix task dev
vix task test
vix task ci
```

The exact tasks depend on the generated template.

## Useful project commands

Inside the project folder:

```bash
vix build
vix run
vix dev
vix check
vix tests
vix fmt
```

| Command     | Purpose                            |
| ----------- | ---------------------------------- |
| `vix build` | Compile the project.               |
| `vix run`   | Build if needed and start the app. |
| `vix dev`   | Start development mode.            |
| `vix check` | Validate the project.              |
| `vix tests` | Run tests.                         |
| `vix fmt`   | Format source files.               |

These commands are the basic lifecycle of a Vix.cpp project.

## Project templates

The `--app` template is the simplest way to start.

Vix.cpp also provides templates for more specific project types.

| Template    | Command                            | Use when                                               |
| ----------- | ---------------------------------- | ------------------------------------------------------ |
| Application | `vix new hello --app`              | You want a minimal runnable Vix application.           |
| Backend     | `vix new api --template backend`   | You want a production-oriented API or backend service. |
| Web         | `vix new site --template web`      | You want server-rendered HTML with Vix templates.      |
| Vue         | `vix new dashboard --template vue` | You want a Vue frontend with a Vix C++ backend.        |
| Game        | `vix new game --template game`     | You want a game-oriented Vix project.                  |
| Library     | `vix new mathlib --lib`            | You want a reusable C++ library.                       |

Getting Started uses `--app` because it is the smallest project shape.

The other templates are useful when the application has a clearer direction from the beginning.

## When to use another template

Use the backend template when you want a backend structure with routes, controllers, middleware, storage, public files, tests, configuration, and production diagnostics from the beginning.

Use the web template when you want HTML rendered on the server with Vix templates.

Use the Vue template when you want a modern frontend and a Vix C++ backend in the same project.

Use the game template when you want a game-oriented runtime structure.

Use the library template when the project should produce reusable C++ code instead of a runnable application.

## When to move beyond the simple app

A simple application project is a good starting point, but larger applications need more structure.

Move beyond the minimal app when you need:

- multiple source files
- reusable headers
- route organization
- services or modules
- tests for different components
- static files
- templates
- database access
- production configuration
- packaging
- custom build behavior

At that point, either use a more specific Vix template or introduce a full `CMakeLists.txt` if the build itself needs advanced control.

## Common mistakes

### Running commands outside the project

Wrong:

```bash
cd ~/tmp
vix run
```

Correct:

```bash
cd ~/tmp/hello
vix run
```

Run project commands from the project folder.

### Forgetting `.env`

If the project has `.env.example`, create your local `.env`:

```bash
cp .env.example .env
```

Do this once after generating the project.

### Forgetting to stop the previous server

If port `8080` is already in use, stop the previous server with:

```txt
Ctrl+C
```

Or change the port in `.env`:

```dotenv
SERVER_PORT=3000
```

Then run again:

```bash
vix run
```

### Editing files but not using development mode

For active development, prefer:

```bash
vix dev
```

Use `vix run` when you simply want to start the app.

## What you should remember

Create a simple application project:

```bash
vix new hello --app
cd hello
cp .env.example .env
```

Build it:

```bash
vix build
```

Run it:

```bash
vix run
```

Develop it:

```bash
vix dev
```

A Vix.cpp project is where a quick experiment becomes a structured application.

## Next step

Build your first HTTP server with Vix.cpp.

Next: [Your First HTTP Server](/getting-started/first-http-server)

For deeper project structures, continue with:

- [Application template](/templates/application)
- [Backend template](/templates/backend)
- [Web template](/templates/web)
- [Vue template](/templates/vue)
- [Game template](/templates/game)
- [Library template](/templates/library)
