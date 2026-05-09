# Vix REPL

The Vix REPL is the interactive shell built into the `vix` binary.

Running `vix` without a command starts the REPL by default:

```bash
vix
```

You can also start it explicitly:

```bash
vix repl
```

The REPL gives you a fast scratchpad for experimenting with expressions, variables, JSON values, filesystem helpers, environment values, and Vix CLI commands.

## What the REPL is for

Use the REPL when you want to:

- test small expressions
- evaluate math quickly
- create and inspect variables
- validate JSON data
- inspect the current environment
- call simple Vix runtime helpers
- run Vix commands without leaving the shell
- prototype logic before moving it into a real C++ file

It is inspired by interactive shells from tools like Python, Node.js, and Deno, but adapted to the Vix.cpp workflow.

## Start the REPL

```bash
vix
```

Example startup:

```
Vix.cpp v1.x (CLI) — Modern C++ backend runtime
[GCC 13.3.0] on linux
Exit: Ctrl+C / Ctrl+D | Clear: Ctrl+L | Type help for help
vix>
```

You can also use:

```bash
vix repl
```

To pass arguments into the REPL session:

```bash
vix repl -- --port 8080 --mode dev
```

Inside the REPL, those arguments are available with:

```
Vix.args()
```

## Basic expressions

You can type expressions directly:

```
1 + 2
10 * (3 + 4)
100 / 5
```

The REPL evaluates the expression and prints the result.

## Variables

Assign a value:

```
x = 42
```

Read it back:

```
x
```

Use it in another expression:

```
x + 1
x * 10
```

String values are supported:

```
name = "Gaspard"
name
```

## JSON values

The REPL supports strict JSON values.

### Objects

```json
user = {"name":"Gaspard","age":10}
user
```

### Arrays

```json
nums = [1,2,3,4]
nums
```

### Nested JSON

```json
profile = {
  "name": "Gaspard",
  "meta": {"country": "UG", "verified": true},
  "tags": ["cpp", "vix", "repl"]
}
profile
```

JSON must be valid.

**Wrong:**
```json
{ "name", "Gaspard" }
```

**Correct:**
```json
{ "name": "Gaspard" }
```

## Printing output

Use `print()` to print without forcing a new line:

```
print("Hello")
```

Use `println()` for line-oriented output:

```
println("Hello world")
```

You can mix strings and expressions:

```
x = 3
println("x =", x)
println("x+1 =", x+1)
```

## Built-in helpers

The REPL exposes simple helpers directly and through the built-in `Vix` object.

### Current working directory

```
cwd()
```

or:

```
Vix.cwd()
```

### Change directory

```
Vix.cd("..")
```

### Process id

```
pid()
```

or:

```
Vix.pid()
```

### Environment variables

```
Vix.env("HOME")
Vix.env("PATH")
```

### REPL arguments

```
Vix.args()
```

## Filesystem helpers

Create a directory:

```
Vix.mkdir("tmp")
```

Create nested directories:

```
Vix.mkdir("tmp/logs", true)
```

## Run Vix commands from the REPL

You can run CLI commands without leaving the REPL:

```
Vix.run("version")
Vix.run("help")
Vix.run("check", "--help")
```

This is useful when you want to inspect a command while staying inside an interactive session.

## Clear the screen

Use:

```
clear
```

or press `Ctrl + L`.

## Exit the REPL

Use:

```
exit
```

You can also exit with `Ctrl + D` or `Ctrl + C`.

## Common workflow

Start the REPL:

```bash
vix
```

Try simple values:

```
x = 10
x * 2
```

Inspect the environment:

```
Vix.cwd()
Vix.env("HOME")
```

Check JSON:

```json
user = {"name":"Ada","role":"developer"}
user
```

Run a Vix command:

```
Vix.run("version")
```

Exit:

```
exit
```

## Common mistakes

### Invalid JSON syntax

JSON objects require `key: value` pairs.

**Wrong:**
```json
user = {"name","Gaspard"}
```

**Correct:**
```json
user = {"name":"Gaspard"}
```

### Forgetting quotes around strings

**Wrong:**
```
name = Gaspard
```

**Correct:**
```
name = "Gaspard"
```

### Passing REPL arguments without `--`

**Wrong:**
```bash
vix repl --port 8080
```

**Correct:**
```bash
vix repl -- --port 8080
```

Everything after `--` is passed to the REPL session and becomes available through `Vix.args()`.

## Best practices

- Use the REPL as a scratchpad.
- Use it to validate small ideas before creating a full C++ file.
- Use it to inspect environment variables when debugging CLI behavior.
- Use it to test JSON values before using them in configuration files or APIs.
- Use `Vix.run()` to quickly inspect CLI command help from inside the session.

## Roadmap

Planned improvements include:

- [ ] Property access for JSON objects
- [ ] Function definitions
- [ ] History persistence
- [ ] Autocomplete for variables
- [ ] Structured error hints
- [ ] Module imports

## Next step

Continue with project creation.

[Open the vix new guide](/cli/new)
