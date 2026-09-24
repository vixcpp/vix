# Simple.json examples

These examples demonstrate the usage of `vix::json::Simple`,
a lightweight in-memory JSON-like model.

## When to use Simple.json
- Internal data exchange
- No JSON parsing/serialization needed
- Predictable memory layout
- Cheap copies and mutations

## Files
- `01_basic_values.cpp` – primitive values
- `02_arrays.cpp` – array container API
- `03_objects.cpp` – object container API
- `04_nested.cpp` – nested structures
- `05_mutation.cpp` – dynamic mutation
- `06_iteration.cpp` – iteration helpers
- `07_merge_and_erase.cpp` – advanced object ops

If you need parsing or dumping JSON text, use `<vix/json.hpp>` instead.
