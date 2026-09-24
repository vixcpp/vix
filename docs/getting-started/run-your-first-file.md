# Run Your First C++ File

This page shows how to run a single C++ file with Vix.cpp.

Single-file mode is the fastest way to try Vix.cpp without creating a full project. It is useful for learning, quick experiments, small tools, examples, and testing small pieces of code.

The main command is:

```bash
vix run main.cpp
```

Vix.cpp detects the source file, builds it with the native C++ toolchain, then runs the generated program.

## Create a workspace

Create a clean folder for the example:

```bash
mkdir -p ~/tmp/vix-first-file
cd ~/tmp/vix-first-file
```

## Create `main.cpp`

Create a minimal C++ file:

```bash
cat > main.cpp <<'CPP'
#include <vix.hpp>

int main()
{
  vix::print("Hello from Vix.cpp");
  return 0;
}
CPP
```

Run it:

```bash
vix run main.cpp
```

Expected output:

```txt
Hello from Vix.cpp
```

At this point, you have compiled and executed a native C++ program through the Vix.cpp workflow.

## What happened?

When you run:

```bash
vix run main.cpp
```

Vix.cpp treats `main.cpp` as a single-file program.

It prepares the build, compiles the file, links the executable, and starts it.

You do not need a project folder, a `CMakeLists.txt`, or a `vix.app` file for this mode.

```txt
main.cpp
  -> vix run
  -> native executable
  -> program output
```

This mode is intentionally small. It gives you a direct path from a C++ file to a running program.

## When single-file mode is useful

Use single-file mode when you want to:

- test a small idea
- learn a Vix.cpp API
- write a small local tool
- run a documentation example
- validate a short C++ snippet
- experiment before creating a project

When the code grows into multiple files, tests, dependencies, or a stable application, move to a Vix project.

## Run a small HTTP server

A single file can also use Vix.cpp runtime APIs.

Replace `main.cpp`:

```bash
cat > main.cpp <<'CPP'
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.text("Hello from Vix.cpp");
  });

  app.run();

  return 0;
}
CPP
```

Create a local `.env` file:

```bash
cat > .env <<'EOF'
SERVER_PORT=8080
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
EOF
```

Run the server:

```bash
vix run main.cpp
```

Expected output shape:

```txt
Vix.cpp   READY   v2.6.0
HTTP:    http://localhost:8080/
Status:  ready
```

Open another terminal and test it:

```bash
curl -i http://127.0.0.1:8080/
```

Expected response body:

```txt
Hello from Vix.cpp
```

Stop the server with:

```txt
Ctrl+C
```

## Why `.env` is used

The server port is stored in `.env` so the source code stays focused on application logic.

```dotenv
SERVER_PORT=8080
```

Then the code can simply call:

```cpp
app.run();
```

This is better than hardcoding the port in every example when the application is meant to behave like a real project later.

## Return JSON

Most backend services return JSON.

Replace `main.cpp` with:

```bash
cat > main.cpp <<'CPP'
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.json({
      "message", "Hello from Vix.cpp",
      "mode", "single-file"
    });
  });

  app.get("/health", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "service", "first-file"
    });
  });

  app.run();

  return 0;
}
CPP
```

Run it:

```bash
vix run main.cpp
```

Test both routes:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
```

Expected response shape for `/`:

```json
{
  "message": "Hello from Vix.cpp",
  "mode": "single-file"
}
```

Expected response shape for `/health`:

```json
{
  "ok": true,
  "service": "first-file"
}
```

## Add a route parameter

Route parameters let the application read values from the URL.

Update `main.cpp`:

```bash
cat > main.cpp <<'CPP'
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.json({
      "message", "Hello from Vix.cpp",
      "mode", "single-file"
    });
  });

  app.get("/health", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "service", "first-file"
    });
  });

  app.get("/hello/{name}", [](Request &req, Response &res) {
    const std::string name = req.param("name");

    res.json({
      "greeting", "Hello " + name,
      "powered_by", "Vix.cpp"
    });
  });

  app.run();

  return 0;
}
CPP
```

Run it:

```bash
vix run main.cpp
```

Test it:

```bash
curl -i http://127.0.0.1:8080/hello/Gaspard
```

Expected response shape:

```json
{
  "greeting": "Hello Gaspard",
  "powered_by": "Vix.cpp"
}
```

The value `Gaspard` comes from this route segment:

```txt
/hello/{name}
```

And is read with:

```cpp
req.param("name")
```

## Add a query parameter

Query parameters are values passed after `?` in the URL.

Update `main.cpp` again:

```bash
cat > main.cpp <<'CPP'
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.json({
      "message", "Hello from Vix.cpp",
      "mode", "single-file"
    });
  });

  app.get("/health", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "service", "first-file"
    });
  });

  app.get("/hello/{name}", [](Request &req, Response &res) {
    const std::string name = req.param("name");

    res.json({
      "greeting", "Hello " + name,
      "powered_by", "Vix.cpp"
    });
  });

  app.get("/users/{id}", [](Request &req, Response &res) {
    const std::string id = req.param("id");
    const std::string page = req.query_value("page", "1");

    res.json({
      "id", id,
      "page", page
    });
  });

  app.run();

  return 0;
}
CPP
```

Run it:

```bash
vix run main.cpp
```

Test it:

```bash
curl -i "http://127.0.0.1:8080/users/42?page=2"
```

Expected response shape:

```json
{
  "id": "42",
  "page": "2"
}
```

Here:

```cpp
req.param("id")
```

reads the route parameter:

```txt
/users/{id}
```

And:

```cpp
req.query_value("page", "1")
```

reads the query parameter. If `page` is missing, the default value is `"1"`.

## Pass runtime arguments

Runtime arguments are arguments passed to your program after it starts.

Create a small argument example:

```bash
cat > main.cpp <<'CPP'
#include <vix.hpp>

int main(int argc, char **argv)
{
  vix::print("argc = {}", argc);

  for (int i = 0; i < argc; ++i)
  {
    vix::print("argv[{}] = {}", i, argv[i]);
  }

  return 0;
}
CPP
```

Run it with program arguments:

```bash
vix run main.cpp --run --name Vix
```

The arguments after `--run` are passed to your program, not to the compiler.

## Pass compiler and linker flags

Use `--` when you want to pass flags to the compiler or linker.

Optimization flags:

```bash
vix run main.cpp -- -O2 -DNDEBUG
```

Include paths:

```bash
vix run main.cpp -- -I./include
```

Libraries:

```bash
vix run main.cpp -- -lssl -lcrypto
```

Everything after `--` is treated as a compiler or linker option in single-file mode.

## `--run` vs `--`

Use `--run` for arguments passed to the program:

```bash
vix run main.cpp --run --name Vix
```

Use `--` for compiler or linker flags:

```bash
vix run main.cpp -- -O2 -DNDEBUG
```

These two separators solve different problems.

```txt
--run  -> program arguments
--     -> compiler and linker flags
```

## Use watch mode

During development, you can ask Vix.cpp to rebuild and restart when the file changes:

```bash
vix run main.cpp --watch
```

This is useful for a small single-file program.

For full projects, prefer:

```bash
vix dev
```

## Use sanitizers

Sanitizers help detect memory errors and undefined behavior.

For memory-related debugging:

```bash
vix run main.cpp --san
```

For undefined behavior checks:

```bash
vix run main.cpp --ubsan
```

Use these options when debugging suspicious behavior or validating code before moving it into a larger project.

## When to create a project

Single-file mode is useful, but it is not meant to replace a project structure.

Move to a project when you need:

- multiple source files
- headers
- dependencies
- tests
- `.env.example`
- stable configuration
- reusable tasks
- packaging
- production builds

Create a project with:

```bash
vix new hello --app
cd hello
vix dev
```

A project gives the code a stable place to grow.

## Common mistakes

### Passing runtime arguments with `--`

Wrong:

```bash
vix run main.cpp -- --name Vix
```

Correct:

```bash
vix run main.cpp --run --name Vix
```

Use `--run` for program arguments.

### Passing compiler flags with `--run`

Wrong:

```bash
vix run main.cpp --run -O2
```

Correct:

```bash
vix run main.cpp -- -O2
```

Use `--` for compiler and linker flags.

### Forgetting `.env` for the server port

If the HTTP example does not start on the expected port, check the `.env` file:

```dotenv
SERVER_PORT=8080
```

Then run again:

```bash
vix run main.cpp
```

### Keeping too much code in one file

Single-file mode is for small programs and examples.

When the file grows too much, move to a project:

```bash
vix new hello --app
cd hello
```

Then split the code into `src/`, `include/`, and `tests/`.

## What you should remember

Run a single C++ file:

```bash
vix run main.cpp
```

Use `.env` for runtime configuration:

```dotenv
SERVER_PORT=8080
```

Use `--run` for program arguments:

```bash
vix run main.cpp --run --name Vix
```

Use `--` for compiler and linker flags:

```bash
vix run main.cpp -- -O2 -DNDEBUG
```

Move to a project when the code becomes an application.

## Next step

Create your first Vix project.

Next: [Create Your First Project](/getting-started/create-your-first-project)
