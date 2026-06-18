# Set Up Your Environment

This page helps you verify that your local machine is ready to build and run Vix.cpp applications.

At this point, Vix.cpp should already be installed. The goal here is not to repeat the installation process, but to confirm that the command-line tool, SDK, compiler, build tools, and runtime configuration are working together.

A working Vix.cpp environment usually includes:

- the `vix` CLI
- the Vix.cpp SDK
- a C++ compiler
- CMake
- Ninja
- the required system libraries for your platform

Once these pieces are available, you should be able to run a C++ file, start a small HTTP application, and inspect your setup with Vix commands.

## Check the Vix CLI

Start by confirming that the `vix` command is available:

```bash
vix --version
```

Expected output shape:

```txt
Vix.cpp CLI
version : 2.6.0
author  : Gaspard Kirira
source  : https://github.com/vixcpp/vix
```

The exact version may be newer depending on the release you installed.

If the command is not found, your shell may not have the Vix installation directory in `PATH`, or the installation may not have completed correctly.

## Inspect the installation

Use:

```bash
vix info
```

This command shows useful information about the current Vix.cpp installation, such as paths, cache locations, and local environment details.

You can also run:

```bash
vix doctor
```

Use `vix doctor` when you want Vix.cpp to check whether the environment looks healthy.

These commands are especially useful when a project behaves differently across machines.

## Choose a working folder

Use a clean folder for the first examples.

Temporary workspace:

```bash
mkdir -p /tmp/vix-env-check
cd /tmp/vix-env-check
```

Or use your normal projects directory:

```bash
mkdir -p ~/projects/vix-examples
cd ~/projects/vix-examples
```

A clean folder makes it easier to understand which files are part of the example.

## Create a small Vix application

Create `main.cpp`:

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
      "mode", "environment-check"
    });
  });

  app.run();

  return 0;
}
CPP
```

This is a normal C++ source file. It includes the main Vix.cpp header, creates an application, registers one route, and starts the server.

The call to `app.run()` is intentional. The server configuration will come from the environment instead of being hardcoded in the source file.

## Configure the application with `.env`

Create a local `.env` file:

```bash
cat > .env <<'EOF'
SERVER_PORT=8080
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
EOF
```

Your folder now contains:

```txt
main.cpp
.env
```

The `.env` file keeps local configuration outside the source code.

This matters because the same application code can run with different settings on different machines or in production.

## Run the application

Run the file:

```bash
vix run main.cpp
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

This means Vix.cpp successfully prepared the build, compiled the program, linked it, and started the application.

The first run may take longer than later runs because the build environment may need to be prepared.

## Test the server

Open another terminal and run:

```bash
curl http://127.0.0.1:8080/
```

Expected response shape:

```json
{
  "message": "Hello from Vix.cpp",
  "mode": "environment-check"
}
```

You can also open this URL in a browser:

```txt
http://localhost:8080/
```

If you receive the JSON response, your environment is ready to run a basic Vix.cpp application.

## Stop the server

Return to the terminal running the server and press:

```txt
Ctrl+C
```

Expected output shape:

```txt
Program interrupted by user (SIGINT).
```

The exact shutdown output may vary, but the important part is that the application stops cleanly.

## Change the port

To run the same application on another port, edit `.env`:

```dotenv
SERVER_PORT=3000
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
```

Run it again:

```bash
vix run main.cpp
```

Open:

```txt
http://localhost:3000/
```

The source code does not need to change.

This is the recommended pattern for normal applications: keep environment-specific values outside the code.

## Recommended editor setup

You can use any editor that supports C++.

Common options include:

| Tool          | Recommendation                                           |
| ------------- | -------------------------------------------------------- |
| Editor        | VS Code, CLion, Vim, Neovim, or Zed                      |
| Compiler      | GCC or Clang on Linux/macOS, MSVC or clang-cl on Windows |
| Build system  | CMake                                                    |
| Build backend | Ninja                                                    |
| Terminal      | Bash, Zsh, PowerShell, or Windows Terminal               |

For VS Code, useful extensions include:

- C/C++
- CMake Tools
- clangd, optional

For larger projects, make sure your editor can read `compile_commands.json` when available. This improves code navigation, diagnostics, completion, and refactoring support.

## Recommended Git setup

If you plan to create real projects, configure Git:

```bash
git config --global user.name "Your Name"
git config --global user.email "you@example.com"
```

Check the current Git configuration:

```bash
git config --global --list
```

This step is not required to run Vix.cpp, but it is useful before creating and committing projects.

## Environment variables

Vix.cpp applications should keep local configuration in environment variables when possible.

For local development, use `.env`:

```dotenv
SERVER_PORT=8080
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
```

For production, these values can come from a service manager, deployment platform, container runtime, CI system, or system environment.

The source code stays the same. Only the environment changes.

This makes applications easier to move between local development, staging, and production.

## Common issues

### `vix` command not found

If this fails:

```bash
vix --version
```

Your shell cannot find the Vix executable.

Check your installation, then open a new terminal and run the command again.

On Unix-like systems, also check:

```bash
echo $PATH
```

The Vix installation directory must be available in `PATH`.

### Port 8080 is already in use

If another program already uses port `8080`, change the port in `.env`:

```dotenv
SERVER_PORT=3000
```

Then run again:

```bash
vix run main.cpp
```

On Linux/macOS, you can inspect the process using the port:

```bash
sudo lsof -i :8080
```

### The app starts but `curl` cannot connect

Make sure the server is still running.

You should see output similar to:

```txt
Vix.cpp   READY
```

Then test again:

```bash
curl http://127.0.0.1:8080/
```

If you changed the port in `.env`, use that port instead.

### The first run is slower

The first run may take longer because Vix.cpp may need to configure and build the application.

Later runs are usually faster because build metadata and intermediate outputs can be reused.

### The editor does not understand headers

If your editor cannot find `vix.hpp`, first confirm that the CLI can build the file:

```bash
vix run main.cpp
```

If the command works, the issue is probably editor configuration, not the installed SDK.

For larger projects, configure the editor to use the generated build metadata, especially `compile_commands.json` when available.

## What you should remember

Check the CLI:

```bash
vix --version
```

Inspect the environment:

```bash
vix info
vix doctor
```

Run a local application:

```bash
vix run main.cpp
```

Keep configuration outside the code:

```dotenv
SERVER_PORT=8080
```

If the server prints a `READY` message and responds to `curl`, your environment is ready.

## Next step

Run your first C++ file with Vix.cpp.

Next: [Run Your First C++ File](/getting-started/run-your-first-file)
