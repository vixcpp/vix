# Allocation and No-Exception Policy

## One process-allocation rule

Vix does not encode process C++ heap exhaustion as an operational `Error`.
That condition can prevent reliable construction of the diagnostics and values
needed to report an `Error` in the first place.

For allocations controlled inside a Vix public operation, heap exhaustion is a
fatal allocation condition: Vix terminates deterministically. This is the same
semantic outcome in exception-enabled and exception-disabled builds.

In an exception-enabled build, the implementation catches `std::bad_alloc` at
the controlled internal allocation boundary and terminates. In an
exception-disabled build, allocation failure reaches the implementation's
configured allocation-failure path and terminates. No Vix API may turn one
case into `Result` Failure while treating the other as fatal.

An allocation performed by the caller before an API is entered remains ordinary
C++ behavior. For example, creating a temporary `std::string` argument may
fail before `Path::from_utf8` can apply any Vix contract.

## Provider resource exhaustion

Provider-mediated exhaustion can be an ordinary operational Failure if the
domain defines a stable semantic identity for it. A future Files
`resource_exhausted` identity means exhaustion of a filesystem/provider
resource, such as file descriptors/handles or a provider resource limit. It
does **not** mean process heap exhaustion.

Likewise, storage exhaustion, quota exhaustion, and read-only storage are
separate Files conditions where the Files contract distinguishes them. They do
not become aliases for heap exhaustion.

## Files baseline implementation gate

The Files baseline has no implementation in this repository yet. Its eventual
implementation MUST compile its public headers and a representative use probe
with exceptions disabled, without changing any declared result or lifecycle
semantics. The probe must cover at least:

```text
Path UTF-8 construction, copy, move, display, lexical transformation
OpenOptions, ReadLimit, WriteOptions, CopyOptions, RemoveTreeOptions moves
File and DirectoryIterator moves
whole-file read result construction
DirectoryEntry and FileInfo value construction
```

The probe is intentionally an implementation-acceptance gate rather than a
placeholder Files test: adding it now would require inventing Files headers or
types before the approved implementation begins.

## Allocation classification for the Files baseline

| Surface | Allocation classification | Exhaustion outcome |
|---|---|---|
| `Path` UTF-8/native construction, copy, UTF-8 conversion, `display`, `joined`, `lexically_normal`, optional component values | May allocate | Fatal allocation condition; never a Files `Error` |
| `File::open` | Provider-dependent allocation/resource acquisition | Heap exhaustion is fatal; provider resource exhaustion is the documented Files Failure |
| `read_all` / `read_utf8` | May allocate result storage and may acquire provider buffers | Heap exhaustion is fatal; read, limit, UTF-8, and provider failures use documented Files identities |
| Whole-file write/copy/rename/remove | Provider-dependent allocation/resource acquisition | Heap exhaustion is fatal; provider/filesystem failure uses documented Files identities |
| `DirectoryIterator` creation and `next`/entry materialization | Provider-dependent and may allocate entry/path values | Heap exhaustion is fatal; provider/filesystem failure uses documented Files identities |
| `FileInfo` / `DirectoryEntry` construction | May allocate only when their contained value representation does | Fatal allocation condition |
| `Error` diagnostics, causes, contexts | May allocate | Fatal allocation condition |

No row promises allocation-free behavior unless the concrete public contract
does so. `Path`, `File`, `DirectoryIterator`, and option move operations that
are declared `noexcept` impose an implementation constraint: their selected
representations and moves MUST NOT allocate or throw. Implementations must not
weaken those declarations to accommodate a convenient storage choice.
