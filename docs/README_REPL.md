# 🧠 Vix REPL — Interactive Runtime Shell

The **Vix REPL** is an interactive shell built directly into the `vix` binary.
Just like **python**, **node**, or **deno**, you start it simply by typing:

```bash
vix
```

No subcommand. No flags.  
This is the **default interactive mode** of Vix.

---

## ✨ What is the Vix REPL?

The Vix REPL is a **developer-friendly interactive environment** designed to:

- Experiment with C++-like expressions
- Test runtime logic quickly
- Evaluate math expressions
- Manipulate variables and JSON data
- Call built-in Vix runtime APIs
- Prototype logic before moving to real code

It feels familiar if you’ve used:

- Python REPL
- Node.js REPL
- Deno REPL

…but adapted to the **Vix.cpp philosophy**.

---

## ▶️ Starting the REPL

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

---

## 🧮 Math Expressions

You can type expressions directly:

```text
1 + 2
10 * (3 + 4)
```

With variables:

```text
x = 3
x + 1
x * 10
```

---

## 📦 Variables

### Assign values

```text
x = 42
name = "Gaspard"
```

### Print variables

```text
x
name
```

---

## 🧩 JSON Support

The REPL supports **strict JSON** using `nlohmann::json`.

### 1. Simple Objects
```text
user = {"name":"Gaspard","age":10}
```

### 2. Arrays
```text
items = [1, 2, 3]
```

### 3. Nested Objects & Arrays
```text
profile = {
  "name": "Gaspard",
  "meta": { "country": "UG", "verified": true },
  "tags": ["cpp", "vix", "repl"]
}
```

### 4. Array of Objects
```text
users = [
  { "id": 1, "name": "Alice" },
  { "id": 2, "name": "Bob" }
]
```

### 5. Mixed Types
```text
config = {
  "active": true,
  "threshold": 3.14,
  "backup": null
}
```

### ❓ Troubleshooting & Common Errors

The JSON parser is **strict**. Here are common syntax mistakes:

| Error Type | Invalid Syntax ❌ | Correct Syntax ✅ |
| :--- | :--- | :--- |
| **Missing Colon** | `{"name" "Gaspard"}` | `{"name": "Gaspard"}` |
| **Comma instead of Colon** | `{"name", "Gaspard"}` | `{"name": "Gaspard"}` |
| **Trailing Comma** | `{"a": 1,}` | `{"a": 1}` |
| **Single Quotes** | `{'name': 'Gaspard'}` | `{"name": "Gaspard"}` |

---

## 🖨️ print / println

### Basic output

```text
print("Hello")
println("Hello world")
```

### Mix strings and expressions

```text
x = 3
println("x =", x)
println("x+1 =", x+1)
```

---

## ⚙️ Built-in Vix API

The REPL exposes a built-in `Vix` object.

### Working directory

```text
cwd()
Vix.cwd()
```

### Change directory

```text
Vix.cd("..")
```

### Process info

```text
pid()
Vix.pid()
```

### Environment variables

```text
Vix.env("HOME")
Vix.env("PATH")
```

### Arguments

```text
Vix.args()
```

---

## 🛠️ Filesystem helpers

```text
Vix.mkdir("tmp")
Vix.mkdir("tmp/logs", true)
```

---

## ▶️ Running CLI commands

You can run CLI commands **from inside the REPL**:

```text
Vix.run("version")
Vix.run("help")
Vix.run("check", "--help")
```

---

## 🧹 Session control

### Clear screen

```text
clear
```

or:

```text
Ctrl + L
```

### Exit REPL

```text
exit
```

or:

```text
Ctrl + D
Ctrl + C
```

---

## 🧠 Tips & Best Practices

- Use the REPL to **prototype logic**
- Validate math & JSON before writing C++
- Use `println()` for debugging expressions
- Treat the REPL as your **scratchpad**

---

## 🧭 Roadmap (REPL)

Planned features:

- Property access: `user.name`
- Function definitions
- History persistence
- Autocomplete for variables
- Structured error hints
- Module imports

---

## 🧾 License

MIT License © Gaspard Kirira  
Part of the **Vix.cpp** ecosystem
