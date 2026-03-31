# vix dev

Developer-friendly entrypoint for running Vix applications with auto-reload.

Internally equivalent to:

```
vix run --watch
```

But optimized for development workflows.

---

## Usage

```bash
vix dev [name] [options] [-- app-args...]
```

---

## Description

`vix dev`:

- Configures the project (if needed)
- Builds it
- Runs it
- Watches for file changes
- Automatically rebuilds and restarts

Works for:

- CMake projects
- Single .cpp scripts
- .vix manifests

---

## Options

```
--force-server           Force classification as long-lived server
--force-script           Force classification as short-lived script
--watch, --reload        Enable hot reload (default in dev)
-j, --jobs <n>           Parallel compile jobs
--log-level <level>      trace | debug | info | warn | error | critical
--verbose                Shortcut for debug logs
-q, --quiet              Show warnings and errors only
```

---

## Examples

Run current project:

```bash
vix dev
```

Run named app:

```bash
vix dev api
```

Script mode with reload:

```bash
vix dev server.cpp
```

Pass runtime args (after `--`):

```bash
vix dev server.cpp -- --port 8080
```

Force server mode:

```bash
vix dev server.cpp --force-server
```

Force script mode:

```bash
vix dev tool.cpp --force-script
```

---

## Mode Classification

Vix automatically detects whether your app is:

- Server-like (long-running)
- Script-like (short-lived)

You can override this behavior with:

```
--force-server
--force-script
```

---

## When To Use vix dev

- Backend API development
- Rapid prototyping
- Iterating on async servers
- Testing script logic
- Live development with auto-rebuild

---

## Design Goals

- Zero friction development loop
- Automatic rebuild on change
- Clean output
- Same behavior across project and script modes

---

`vix dev` is the recommended command during development.

