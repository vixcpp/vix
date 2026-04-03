# Vix REPL

> **Interactive. Dynamic. Expressive.**

## Overview

The Vix REPL is an interactive runtime environment that lets you execute commands, inspect values, and interact with the system in real time. It is:

- **dynamic** (no compilation required)
- **lightweight**
- **expressive**
- designed for real-world usage

> The REPL is not a C++ interpreter. It is a runtime shell built on top of Vix, focused on usability and speed.

---

## Getting Started

Start the REPL:

```sh
vix repl
```

You can now type expressions and commands.

---

## Core Concepts

### 1. Values

The REPL works with dynamic runtime values:

| Type | Description |
|------|-------------|
| `string` | Text value |
| `int` | Integer number |
| `double` | Floating-point number |
| `bool` | Boolean (`true` / `false`) |
| `null` | Null value |
| `array` | JSON array |
| `object` | JSON object |

**Examples:**

```
"hello"
42
3.14
true
null
```

### 2. Variables

You can assign variables:

```
name = "Gaspard"
age = 25
price = 19.99
```

Use them:

```
println(name)
println(age)
```

### 3. Expressions

You can evaluate expressions:

```
println(1 + 2)
println(3 * (2 + 1))
```

Expressions are resolved dynamically at runtime.

---

## Built-in Functions

### `print` / `println`

```
print("hello")
println("hello world")
println("a", "b", "c")
```

### `eprint` / `eprintln`

Print to stderr:

```
eprint("error")
eprintln("fatal error")
```

### `len()`

Returns the length of a string, array, or object:

```
len("hello")    // 5
len([1,2,3])    // 3
len({"a":1})    // 1
```

### `str()`

Converts a value to string:

```
str(42)         // "42"
str(true)       // "true"
str({"a":1})    // {"a":1}
```

### `int()`

Converts to integer:

```
int(42.0)       // 42
int("123")      // 123
int("abc")      // error
```

### `float()`

Converts to floating point:

```
float(10)       // 10.0
float("3.14")   // 3.14
```

### `type()`

Returns the type of a value:

```
type("hello")   // string
type(42)        // int
type(3.14)      // double
type(true)      // bool
type([1,2])     // array
type({"a":1})   // object
type(null)      // null
```

---

## Objects and Arrays

### Object creation

```
user = {"name":"Gaspard","age":25}
```

### Array creation

```
nums = [10, 20, 30]
```

### Property access

```
user.name
user["name"]
```

### Array access

```
nums[0]
nums[1]
```

### Nested access

```
user = {"tags":["cpp","vix"]}
user.tags[0]      // "cpp"
user["tags"][1]   // "vix"
```

### Direct evaluation

You can type expressions directly:

```
user.name
nums[2]
```

---

## Vix API

The REPL exposes system-level functionality via `Vix`.

### `Vix.cd(path)`

Change directory:

```
Vix.cd("..")
Vix.cd("/home/user")
```

### `Vix.cwd()`

Print current directory:

```
Vix.cwd()
```

### `Vix.mkdir(path, recursive?)`

```
Vix.mkdir("test")
Vix.mkdir("a/b/c", true)
```

### `Vix.env(key)`

Read an environment variable:

```
Vix.env("HOME")
```

Returns: `string` or `null` if not found.

### `Vix.pid()`

```
Vix.pid()
```

### `Vix.exit(code?)`

Exit the REPL:

```
Vix.exit()
Vix.exit(0)
```

### `Vix.args()`

Returns CLI arguments:

```
Vix.args()
```

### `Vix.history()`

Access command history:

```
Vix.history()
```

### `Vix.history_clear()`

Clear history:

```
Vix.history_clear()
```

---

## Examples

### Example 1

```
name = "Gaspard"
println("Hello", name)
```

### Example 2

```
nums = [10, 20, 30]
println(len(nums))
println(nums[1])
```

### Example 3

```
user = {
  "name": "Gaspard",
  "age": 25,
  "skills": ["cpp", "vix"]
}
println(user.name)
println(user.skills[0])
```

### Example 4

```
println(int("42"))
println(float("3.14"))
println(type(user))
```

---

## Error Handling

Errors are displayed as:

```
error: message
```

**Examples:**

```
len(42)       // error: len() expects string, array or object
Vix.cd(42)    // error: Vix.cd(path:string)
```

---

## Design Philosophy

The Vix REPL is:

- **dynamic**, not statically typed
- **simple**, not verbose
- **practical**, not theoretical

It is designed to:

- explore data quickly
- interact with the system
- debug workflows
- prototype logic

---

## Summary

The Vix REPL combines:

- dynamic values
- JSON-native structures
- system APIs
- expressive syntax

All in a **fast, minimal, interactive environment**.

