# Pull Request Checklist

Thank you for contributing to **Vix.cpp**.  
Please review this checklist before submitting your Pull Request.

---

## Summary

### What does this PR do?

<!-- Briefly describe the change and its purpose -->

### Related issues

<!-- Link related issues, e.g. Fixes #123 -->

---

## Type of Change

Please check the relevant option(s):

- [ ] New feature
- [ ] Bug fix
- [ ] Refactoring (no behavior change)
- [ ] Performance improvement
- [ ] Documentation update
- [ ] Build / tooling / CI change
- [ ] Other (please describe):

---

## Code Quality

- [ ] Code follows the project style and conventions
- [ ] C++20 (or newer) features are used appropriately
- [ ] No unnecessary complexity introduced
- [ ] No unused code, headers, or debug logs
- [ ] Public APIs are documented where applicable

---

## Tests

- [ ] Existing tests pass locally
- [ ] New tests were added for new behavior or fixes
- [ ] Tests are located under `tests/` or `unittests/`
- [ ] `ctest --output-on-failure` passes

---

## Performance (if applicable)

- [ ] Change has no negative performance impact
- [ ] Benchmarks were run for performance-sensitive changes
- [ ] Results are included or described in the PR

---

## CLI / UX (if applicable)

- [ ] CLI output is clear and consistent
- [ ] Help (`-h` / `--help`) output was updated if needed
- [ ] Exit codes follow project conventions

---

## Documentation

- [ ] Documentation was updated (README / docs)
- [ ] Examples were added or updated if relevant
- [ ] Comments explain non-obvious logic

---

## CMake / Build System

- [ ] CMakeLists changes are minimal and scoped
- [ ] Uses `target_*` commands only (no global includes)
- [ ] Targets are exported under the `Vix::` namespace

---

## Final Checks

- [ ] I have rebased on the latest `dev` branch
- [ ] Commit messages follow Conventional Commits
- [ ] This PR is ready for review

---

## Additional Notes

<!-- Add any additional context, trade-offs, or follow-up work -->

---

By submitting this Pull Request, I confirm that my contribution is licensed
under the same **MIT License** as the Vix.cpp project.
