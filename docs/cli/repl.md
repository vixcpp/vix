# Vix Reply

Interactive REPL engine for Vix.

Vix Reply powers the `vix` and `vix repl` interactive experience. It provides a fast shell for testing expressions, variables, JSON values, runtime helpers, and real C++ snippets powered by the Vix run pipeline.

## Start the REPL

Running `vix` without a command starts the REPL by default:

```bash
vix
```

You can also start it explicitly:

```bash
vix repl
```

To pass arguments into the REPL session, use `--`:

```bash
vix repl -- --port 8080 --mode dev
```

Inside the REPL, those arguments are available with:

```text
Vix.args()
```

## Example startup

```text
Vix Reply v2.5.6  REPL
gcc 13.3  linux
exit: Ctrl+D | clear: Ctrl+L | help

>>>
```

## What the REPL is for

Use Vix Reply when you want to:

- test small expressions
- evaluate math quickly
- create and inspect variables
- validate JSON values
- inspect environment values
- call simple Vix runtime helpers
- run real C++ snippets from an interactive session
- prototype small ideas before moving them into a C++ file

Vix Reply is inspired by interactive shells from tools like Python, Node.js, and Deno, but adapted to the Vix.cpp workflow.

## Basic commands

```text
help
version
pwd
cd <dir>
clear
history
history clear
exit
```

You can also clear the screen with:

```text
Ctrl+L
```

To exit, use:

```text
exit
```

or:

```text
Ctrl+D
```

## Expressions

You can type expressions directly:

```text
>>> 1 + 2
3

>>> 10 * (3 + 4)
70

>>> 100 / 5
20
```

You can also use `calc` explicitly:

```text
>>> calc 10 * (2 + 3)
50
```

## Variables

Assign a value:

```text
>>> x = 42
42
```

Read it back:

```text
>>> x
42
```

Use it in another expression:

```text
>>> x + 1
43

>>> x * 10
420
```

String values are supported:

```text
>>> name = "Gaspard"
name = "Gaspard"

>>> name
Gaspard
```

Other simple values are supported too:

```text
>>> age = 25
age = 25

>>> price = 19.99
price = 19.99

>>> active = true
active = true
```

## JSON values

Vix Reply supports strict JSON values.

### Objects

```text
>>> user = {"name":"Gaspard","age":25}
user = {"age":25,"name":"Gaspard"}

>>> user
{"age":25,"name":"Gaspard"}
```

### Arrays

```text
>>> nums = [10,20,30]
nums = [10,20,30]

>>> nums
[10,20,30]
```

### Nested JSON

```text
>>> profile = {"name":"Gaspard","meta":{"country":"UG","verified":true},"tags":["cpp","vix","reply"]}
profile = {"meta":{"country":"UG","verified":true},"name":"Gaspard","tags":["cpp","vix","reply"]}
```

Access object properties:

```text
>>> user.name
Gaspard

>>> user["name"]
Gaspard
```

Access arrays:

```text
>>> nums[0]
10

>>> profile.tags[1]
vix
```

JSON must be valid.

Wrong:

```json
{"name","Gaspard"}
```

Correct:

```json
{ "name": "Gaspard" }
```

## Printing output

Use `print()`:

```text
>>> print("Hello")
Hello
```

Use `println()`:

```text
>>> println("Hello world")
Hello world
```

You can mix strings and values:

```text
>>> x = 3
x = 3

>>> println("x =", x)
x = 3

>>> println("x + 1 =", x + 1)
x + 1 = 4
```

Supported examples:

```text
print("hello")
println("hello", "world")
println(42)
println(-42)
println(3.14)
println(true)
println(null)
```

## Built-in helpers

Vix Reply exposes simple helpers directly and through the built-in `Vix` object.

### Current working directory

```text
>>> cwd()
/home/user/project
```

or:

```text
>>> Vix.cwd()
/home/user/project
```

### Change directory

```text
>>> Vix.cd("..")
```

### Process id

```text
>>> pid()
12345
```

or:

```text
>>> Vix.pid()
12345
```

### Environment variables

```text
>>> Vix.env("HOME")
/home/user

>>> Vix.env("PATH")
/usr/local/bin:/usr/bin:/bin
```

### REPL arguments

```text
>>> Vix.args()
["--port","8080","--mode","dev"]
```

### Create directories

```text
>>> Vix.mkdir("tmp")
```

Create nested directories:

```text
>>> Vix.mkdir("tmp/logs", true)
```

### Exit with a code

```text
>>> Vix.exit(0)
```

## Value helpers

### Length

```text
>>> name = "Gaspard"
name = "Gaspard"

>>> len(name)
7
```

```text
>>> nums = [10,20,30]
nums = [10,20,30]

>>> len(nums)
3
```

```text
>>> user = {"name":"Gaspard","age":25}
user = {"age":25,"name":"Gaspard"}

>>> len(user)
2
```

### Convert values

```text
>>> int("42")
42

>>> int(42.0)
42

>>> float("3.14")
3.140000

>>> float(10)
10.000000

>>> str(25)
25
```

### Inspect types

```text
>>> type(user)
object

>>> type(nums)
array

>>> type(user.name)
string

>>> type(nums[0])
int
```

## C++ snippet mode

Vix Reply can run real C++ snippets through the normal Vix run pipeline.

Enter C++ mode:

```text
>>> :cpp
C++ mode. Type :run to execute or :cancel to exit.
cpp>
```

Example:

```text
>>> :cpp
C++ mode. Type :run to execute or :cancel to exit.
cpp> #include <vector>
...   #include <vix/print.hpp>
...   int main() {
...     vix::print("Hello, world", 2, true, std::vector<int>{1,2,3});
...   }
Hello, world 2 true [1, 2, 3]
```

## Run a Vix HTTP server from the REPL

You can also start a small Vix HTTP server from C++ snippet mode:

```text
>>> :cpp
C++ mode. Type :run to execute or :cancel to exit.
cpp> #include <vix.hpp>
...   using namespace vix;
...
...   int main() {
...     App app;
...
...     app.get("/", [](Request&, Response& res) {
...       res.send("Hello, world");
...     });
...
...     app.run(8080);
...   }
```

Then test it from another terminal:

```bash
curl http://localhost:8080
```

Expected output:

```text
Hello, world
```

## C++ mode commands

```text
:cpp       Enter C++ snippet mode
:run       Run the current C++ snippet
:cancel    Cancel C++ snippet mode
```

## Important note about C++ mode

Vix Reply is not a full C++ interpreter.

C++ snippet mode writes the snippet to a temporary `.cpp` file and runs it through `vix run`. This means the code is validated by the real C++ compiler and uses the normal Vix build, diagnostics, and runtime behavior.

The snippet must be valid C++ code.

For example, callbacks must use valid C++ lambda syntax:

```cpp
app.get("/", [](Request&, Response& res) {
  res.send("Hello, world");
});
```

## Common mistakes

### Invalid JSON syntax

JSON objects require `key: value` pairs.

Wrong:

```json
{"name","Gaspard"}
```

Correct:

```json
{ "name": "Gaspard" }
```

### Forgetting quotes around strings

Wrong:

```text
>>> name = Gaspard
```

Correct:

```text
>>> name = "Gaspard"
```

### Passing REPL arguments without `--`

Wrong:

```bash
vix repl --port 8080
```

Correct:

```bash
vix repl -- --port 8080
```

Everything after `--` is passed to the REPL session and becomes available through `Vix.args()`.

### Forgetting to close a C++ function call

Wrong:

```cpp
app.get("/", [](Request&, Response& res) {
  res.send("Hello, world");
}
```

Correct:

```cpp
app.get("/", [](Request&, Response& res) {
  res.send("Hello, world");
});
```

## Common workflow

Start the REPL:

```bash
vix
```

Try simple values:

```text
>>> x = 10
10

>>> x * 2
20
```

Inspect the environment:

```text
>>> Vix.cwd()
/home/user/project

>>> Vix.env("HOME")
/home/user
```

Check JSON:

```text
>>> user = {"name":"Ada","role":"developer"}
user = {"name":"Ada","role":"developer"}

>>> user.role
developer
```

Run C++:

```text
>>> :cpp
cpp> #include <vix/print.hpp>
...   int main() {
...     vix::print("Hello from C++");
...   }
Hello from C++
```

Exit:

```text
>>> exit
```

## Best practices

- Use the REPL as a scratchpad.
- Use it to validate small expressions.
- Use it to inspect environment values.
- Use it to test JSON values before using them in config files or APIs.
- Use C++ snippet mode to quickly test real C++ code through the Vix pipeline.
- Move larger code into a real `.cpp` file or project once the idea is validated.

## Module layout

```text
include/vix/reply/
  api/
  console/
  core/

src/
  api/
  console/
  core/
```

## Public entry point

The main entry point is:

```cpp
#include <vix/reply/core/ReplFlow.hpp>

int repl_flow_run(const std::vector<std::string>& replArgs);
```

The Vix CLI uses this entry point to implement:

```bash
vix repl
```

## Next step

Continue with project creation.

[Open the vix new guide](/cli/new)
