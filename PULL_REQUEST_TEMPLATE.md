# Pull Request

## Summary

Describe what this pull request changes.

Example:

```txt
This PR fixes queued cancellation handling in the threadpool module and updates the sanitizer test coverage.
```

## Type of Change

Check all that apply:

- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Refactor
- [ ] Tests
- [ ] Documentation
- [ ] CI / build system
- [ ] Benchmark
- [ ] Performance improvement
- [ ] Security / stability improvement
- [ ] Release preparation

## Affected Modules

Check all modules affected by this PR:

- [ ] core
- [ ] cli
- [ ] json
- [ ] async
- [ ] threadpool
- [ ] websocket
- [ ] template
- [ ] middleware
- [ ] crypto
- [ ] validation
- [ ] conversion
- [ ] db
- [ ] orm
- [ ] cache
- [ ] kv
- [ ] sync
- [ ] p2p
- [ ] game
- [ ] fs
- [ ] io
- [ ] path
- [ ] env
- [ ] os
- [ ] process
- [ ] log
- [ ] error
- [ ] time
- [ ] utils
- [ ] docs
- [ ] CI
- [ ] benchmarks
- [ ] other:

## What Changed?

List the main changes.

-
-
- ***

## Why?

Explain why this change is needed.

- What problem does it solve?
- What behavior was wrong or missing?
- Why is this approach correct?

## Testing

Describe how this PR was tested.

### Local Build

- [ ] I built the project locally.
- [ ] I built the affected module locally.
- [ ] I built with `--build-target all`.

Commands used:

```bash

```

### Unit / Module Tests

- [ ] I ran relevant module tests.
- [ ] I ran all available tests.
- [ ] New tests were added or updated.
- [ ] No tests were needed because this is documentation-only.

Commands used:

```bash

```

### Sanitizers

- [ ] ASan / UBSan build was tested.
- [ ] Not applicable.

Commands used:

```bash

```

### Valgrind

- [ ] Valgrind was tested.
- [ ] Not applicable.

Commands used:

```bash

```

### Static Analysis

- [ ] cppcheck was tested.
- [ ] clang-tidy was tested.
- [ ] Not applicable.

Commands used:

```bash

```

## Benchmarks

Check one:

- [ ] This PR does not affect performance-sensitive code.
- [ ] This PR may affect performance, but no benchmark was required.
- [ ] Benchmarks were run.
- [ ] Benchmark baseline was updated.

Benchmark command:

```bash

```

Benchmark summary:

```txt

```

If official performance numbers are included, confirm:

- [ ] Benchmarks were run in Release mode.
- [ ] Compiler and platform are documented.
- [ ] Results are reproducible.
- [ ] Debug/dev builds were not used for official numbers.

---

## Documentation

- [ ] README updated.
- [ ] Module README updated.
- [ ] Docs updated.
- [ ] CHANGELOG updated.
- [ ] SECURITY_TESTS updated.
- [ ] CONTRIBUTING updated.
- [ ] No documentation changes needed.

Files changed:

```txt

```

## CI Notes

Does this PR change CI behavior?

- [ ] No.
- [ ] Yes.

If yes, explain:

```txt

```

Confirm:

- [ ] CI does not analyze vendored third-party code as project code.
- [ ] CI does not treat benchmark binaries as normal test executables.
- [ ] Optional disabled backends are not analyzed as active build sources.
- [ ] Module-level tests are enabled where needed.
- [ ] Sanitizer-enabled static libraries are linked by sanitizer-enabled test executables.

## Threading / Async Safety

If this PR touches runtime, async, threadpool, scheduler, executor, shutdown, cancellation, or networking code, confirm:

- [ ] Futures are always resolved.
- [ ] Promises are not left pending.
- [ ] Shutdown paths are deterministic.
- [ ] Cancellation paths are tested.
- [ ] Tests do not assume asynchronous work completes immediately.
- [ ] No raw pointer is used after owned object destruction.
- [ ] Not applicable.

Notes:

```txt

```

## API Compatibility

- [ ] No public API changes.
- [ ] Public API added.
- [ ] Public API changed.
- [ ] Public API removed.
- [ ] Behavior changed without API signature changes.

If API changed, explain:

```txt

```

## Breaking Changes

- [ ] No breaking changes.
- [ ] This PR includes breaking changes.

If breaking, explain migration steps:

```txt

```

## Submodule / Module Pointer Checklist

If this repository tracks modules as nested repositories or submodules:

- [ ] Module commit was pushed first.
- [ ] Umbrella pointer was updated.
- [ ] Umbrella commit references the updated module.
- [ ] Not applicable.

Module commit:

```txt

```

Umbrella commit:

```txt

```

## Security Considerations

- [ ] No security impact.
- [ ] This PR improves security.
- [ ] This PR may affect security-sensitive behavior.

If security-sensitive, explain:

```txt

```

## Screenshots / Logs

Add logs, screenshots, benchmark results, or CI output if useful.

```txt

```

## Final Checklist

Before requesting review:

- [ ] Code compiles.
- [ ] Relevant tests pass.
- [ ] New behavior has tests.
- [ ] Sanitizer build was considered.
- [ ] Static analysis was considered.
- [ ] Documentation was updated if needed.
- [ ] Changelog was updated if user-facing behavior changed.
- [ ] Commit messages are clear.
- [ ] PR title is clear.
- [ ] This PR is ready for review.
