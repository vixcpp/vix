# REPL

Vix CLI includes a built-in REPL (Read-Eval-Print Loop).

It allows you to:

- Evaluate expressions
- Test small logic
- Work with variables
- Inspect JSON-like objects
- Explore the environment quickly

---

## Start the REPL

```bash
vix
```

Example output:

```
Vix.cpp v1.x.x (CLI) — Modern C++ backend runtime
Exit: Ctrl+C / Ctrl+D or type exit
>>>
```

---

## Basic Math

You can evaluate expressions directly:

```
>>> 1 + 2
3

>>> 4 / 2
2

>>> 3 * (2 + 4)
18
```

Division by zero is safely handled:

```
>>> 4/0
Error: division by zero
```

---

## Variables

Assign values:

```
>>> x = 3
>>> x
3

>>> x + 1
4

>>> x = x + 7
>>> x
10
```

---

## Arrays

```
>>> nums = [1,2,3,4]
>>> nums
[1, 2, 3, 4]
```

---

## Objects

JSON-like objects are supported:

```
>>> user = {"name": "Gaspard", "age": 26}
>>> user
{"name":"Gaspard","age":26}
```

Nested structures:

```
>>> profile = {"name":"Gaspard","meta":{"country":"UG","verified":true},"tags":["cpp","vix","repl"]}
```

---

## Built-in Commands

```
help                    Show help
help <command>          Help for a command
version                 Show CLI version
pwd                     Print working directory
cd <dir>                Change directory
clear                   Clear screen
history                 Show command history
history clear           Clear history
exit                    Exit REPL
```

---

## History

```
>>> history
1  1+2
2  x=3
3  x+1
```

Clear history:

```
>>> history clear
```

---

## Behavior Notes

- The REPL is stateful within a session.
- Variables are reset when you exit.
- Build/run commands are disabled inside REPL.
- Designed for quick experimentation, not full application execution.

---

## When To Use REPL

- Test math logic
- Validate JSON structures
- Experiment with expressions
- Debug small ideas quickly
- Demonstrate Vix syntax interactively

---

The REPL is intentionally minimal and predictable.

No hidden behavior.
No magic runtime state.
Just explicit evaluation.


