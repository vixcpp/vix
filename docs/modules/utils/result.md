# Result Guide

`Result&lt;T, E&gt;` is a lightweight alternative to exceptions.

It represents either:

- `Ok(T)`   for success
- `Err(E)`  for failure

Designed for:

- deterministic error handling
- explicit control flow
- no exceptions required

---

## Basic usage

```cpp
#include <vix/utils/Result.hpp>
using vix::utils::Result;

Result<int> divide(int a, int b)
{
  if (b == 0)
    return Result<int>::Err("division by zero");

  return Result<int>::Ok(a / b);
}
```

```cpp
auto r = divide(10, 2);

if (r.is_ok())
{
    int value = r.value();
}
else
{
    std::string err = r.error();
}
```

---

## Creating Results

```cpp
auto ok  = Result<int>::Ok(42);
auto err = Result<int>::Err("failure");
```
---

Explicit forms:

```cpp
auto ok2  = Result<int>::FromOk(10);
auto err2 = Result<int>::FromErr("error");
```

---

## Checking State

```cpp
r.is_ok();   // true if success
r.is_err();  // true if error
```

---

## Accessing Values

```cpp
r.value();   // valid only if is_ok()
r.error();   // valid only if is_err()
```

Accessing the wrong variant triggers an assert.

---

## `Result<void>`

Used when success carries no value.

```cpp
Result<void> save()
{
    if (!can_save())
        return Result<void>::Err("permission denied");

    return Result<void>::Ok();
}
```

---

## Design Notes

- Uses a union internally
- No dynamic allocation inside Result itself
- Copy and move supported
- Explicit construction only (no default state)

---

## Philosophy

Result forces error handling to be explicit.

Instead of:

```cpp
throw std::runtime_error("error");
```

You return:

```cpp
return Result<T>::Err("error");
```

Clear. Predictable. Exception-free.

