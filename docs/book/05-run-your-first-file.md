# Run your first file

Now you will run your first C++ file.

```bash
vix run main.cpp
```

## The goal

Understand:
- how to run a single file,
- how script mode works,
- how to pass runtime arguments,
- how to pass compiler flags,
- and how to avoid common mistakes.

## Create a workspace

```bash
mkdir -p ~/tmp/vix-first-file
cd ~/tmp/vix-first-file
touch main.cpp
```

## First C++ program

```cpp
#include <iostream>

int main()
{
  std::cout << "Hello from Vix\n";
  return 0;
}
```

```bash
vix run main.cpp
# Output: Hello from Vix
```

Vix detects a single `.cpp` file → uses **script mode** → compiles → runs. No CMake, no build directories needed.

## Add Vix headers

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
curl -i http://127.0.0.1:8080/
```

Stop with `Ctrl+C`.

> If you see `error: header file not found` for `vix.hpp`, reinstall the full SDK: `curl -fsSL https://vixcpp.com/install.sh | bash`

## Add a JSON route

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

  app.get("/health", [](Request &, Response &res){
    res.json({"ok", true});
  });

  app.run(8080);
  return 0;
}
```

## Add route params

```cpp
app.get("/hello/{name}", [](Request &req, Response &res){
  const std::string name = req.param("name");

  res.json({
      "greeting", "Hello " + name,
      "powered_by", "Vix.cpp"
  });
});
```

```bash
curl -i http://127.0.0.1:8080/hello/Gaspard
```

## Add query params

```cpp
app.get("/users/{id}", [](Request &req, Response &res){
  const std::string id = req.param("id");
  const std::string page = req.query_value("page", "1");

  res.json({
      "id", id,
      "page", page
  });
});
```

```bash
curl -i "http://127.0.0.1:8080/users/42?page=2"
```

## Passing runtime arguments

Use `--run` for runtime arguments:

```bash
vix run main.cpp --run --port 8080
```

> **Do NOT use `--` for runtime args.** In script mode, `--` forwards to the compiler/linker.

```bash
# Wrong — sends --port to the compiler
vix run main.cpp -- --port 8080

# Correct — sends --port to your program
vix run main.cpp --run --port 8080
```

## Passing compiler flags

Use `--` for compiler or linker flags:

```bash
vix run main.cpp -- -O2 -DNDEBUG
vix run main.cpp -- -lssl -lcrypto

# Combine both
vix run main.cpp -- -O2 -DNDEBUG --run hello 123
```

## Sanitizers

```bash
vix run main.cpp --san     # AddressSanitizer + UBSan
vix run main.cpp --ubsan   # UBSan only
```

## SQLite / MySQL support

```bash
vix run main.cpp --with-sqlite
vix run main.cpp --with-mysql
```

## Other script mode options

```bash
vix run main.cpp --auto-deps      # include local Vix dependencies
vix run main.cpp --local-cache    # use local script cache
vix run main.cpp --no-clear       # preserve terminal output
vix run main.cpp --verbose        # debug logs
vix run main.cpp --quiet          # minimal output
vix run main.cpp --watch          # rebuild on file changes
vix run main.cpp --force-server   # treat as long-running server
vix run main.cpp --force-script   # treat as short-lived tool
```

## When to move from one file to a project

A single file is perfect for learning.
Move to a project when you need multiple source files, headers, tests, dependencies, configuration, or a stable app structure:

```bash
vix new api
cd api
vix dev
```

## Common mistakes

### Using CLI-only install

`#include <vix.hpp>` requires the full SDK.

### Passing runtime args after `--`

```bash
# Wrong
vix run main.cpp -- --port 8080

# Correct
vix run main.cpp --run --port 8080
```

### Port already in use

```bash
sudo lsof -i :8080
```

### Running from the wrong directory

Relative paths (like `res.file("public/index.html")`) depend on where you run `vix`.

## What you should remember

The most important command: `vix run main.cpp`

- `--run` → runtime arguments
- `--` → compiler or linker flags
- `--watch` or `vix dev main.cpp` → faster development loop

The core idea:
- start with one file,
- move to a project when the app grows.

## Next chapter

[Next: Create your first project](/book/06-create-your-first-project)
