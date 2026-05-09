# Error Diagnostics

Vix transforms raw compiler, linker, and runtime failures into clearer diagnostics.

```txt
raw error → parse → classify → show useful message → show code frame → show hint
```

## Why diagnostics matter

A beginner may see:
```
fatal error: vix.hpp: No such file or directory
undefined reference to ...
terminate called without an active exception
```

and not know what to do. Vix diagnostics answer three questions:
**what happened, where, and what to try next.**

## Diagnostic layers

Vix handles:

- Compile errors
- Link errors
- Build system errors
- Runtime crashes
- Sanitizer reports
- Known C++ mistakes
- Unclassified failures

## Build error flow

```txt
build log → trim build preamble → parse GCC/Clang errors → run error pipeline
→ deduplicate errors → print top errors → show code frame
```

## Code frames

```txt
main.cpp:8:3

  6 | int main()
  7 | {
> 8 |   std::cout << "Hello"
    |   ^
  9 |   return 0;
 10 | }
```

## Hint system

```txt
error: expected ';'
hint: missing ';' (often the previous line)

error: use of undeclared identifier 'std'
hint: std is not visible here (include the required standard header)
```

## Maximum visible errors

Usually the first 3 distinct errors are shown; repeated ones are grouped:

```txt
(4 similar error(s) hidden)
```

Fix the first error first — many later errors may disappear.

## Common errors and hints

### Runtime argument mistake

```bash
# Wrong — passes args to compiler
vix run main.cpp -- --config app.json
# Error: unrecognized command-line option '--config'
# Hint: use --run for program arguments
vix run main.cpp --run --config app.json
```

### Header not found

```txt
fatal error: vix.hpp: No such file or directory
hint: install the full Vix SDK, not CLI-only mode
```

```bash
find ~/.local/include -name vix.hpp 2>/dev/null
# Expected: ~/.local/include/vix.hpp
```

### Link error

```txt
undefined reference to ...
hint: missing feature flag, library, or dependency not linked
```

```bash
vix run main.cpp --with-sqlite   # for SQLite
vix run main.cpp --with-mysql    # for MySQL
```

### Port already in use

```txt
runtime error: address already in use
hint: port 8080 is already in use (stop the other process or change the port)
```

```bash
sudo lsof -i :8080
# Then: app.run(9090);
```

## Sanitizer diagnostics

```bash
vix run main.cpp --san     # all sanitizers
vix run main.cpp --ubsan   # UBSan only
```

### UBSan

```txt
runtime error: division by zero
hint: division/modulo by zero (undefined behavior). guard denominators
runtime error: out-of-bounds access
hint: index out of bounds (check vector/string indexing)
```

### ASan

```txt
runtime error: heap-buffer-overflow
hint: heap out-of-bounds (check vector/string indexing and sizes)

runtime error: heap-use-after-free
hint: you used memory after it was freed (dangling pointer/reference)

runtime error: double free
hint: the same allocation was freed twice (double owner or duplicate delete/free)
```

### Double free — Rule of Three

```cpp
// Common cause: shallow copy with raw pointer
struct Buffer { int *data{new int[10]}; ~Buffer() { delete[] data; } };
Buffer a;
Buffer b = a;  // both own the same allocation — double free on destruction

// Fix: use std::vector, std::unique_ptr, or define copy ops
```

### LSan

```txt
runtime error: memory leak
hint: free allocations or use RAII/smart pointers (std::vector, std::unique_ptr)
```

### TSan

```txt
runtime error: data race
hint: protect shared state (mutex/atomic), avoid unsynchronized access
```

### MSan

```txt
runtime error: uninitialized memory
hint: initialize variables and ensure buffers are written before read
// Fix: int value{}; instead of int value;
```

## Thread diagnostics

```
runtime error: joinable std::thread destroyed
hint: join the thread, detach intentionally, or use std::jthread
```

```cpp
// Wrong
// destructor calls std::terminate
void run() {
  std::thread t([] {});
}

// Correct
// Or: std::jthread t([] {});
void run() {
  std::thread t([] {});
  t.join();
}
```

## Stack use after scope

```cpp
std::string_view name;
{ std::string local = "Ada"; name = local; }
std::cout << name;  // dangling view

// Error: stack-use-after-scope
// hint: a reference/view/span outlived the object it refers to
```

## Diagnostic output shape

```txt
title

  code frame if available

hint: what to try next
at: file:line
```

## Error pipeline

```txt
parsed compiler errors → try specialized rules → if handled: print friendly diagnostic
                       → otherwise: print generic compiler diagnostic
```

## Design principles

1. **First error matters most:** show the first meaningful error clearly.
2. **Show source context:** prefer code frames over long raw logs.
3. **Hide repeated noise:** deduplicate repeated diagnostics.
4. **Prefer specific explanations:** `division by zero` is better than `runtime error`.
5. **Give the next action:** include a practical hint when possible.
6. **Keep fallback honest:** show the raw log when the error is unknown.
7. **Do not hide C++:** explain errors without removing the need to understand them.

## What you should remember

```txt
raw log → parse → classify → deduplicate → code frame → hint
```

Vix handles:
- compiler errors,
- linker errors,
- runtime crashes,
- sanitizer reports,
- common C++ mistakes.

The core idea: **a good error message should teach the next step.**

Next: [Cache System](/internals/cache-system)
