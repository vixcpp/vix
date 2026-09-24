# Security Policy

# Vix.cpp Security Policy

Security is important for Vix.cpp.

Vix.cpp is a modular C++20 runtime that includes low-level components such as HTTP handling, async execution, thread pools, WebSocket support, cryptography helpers, file and process utilities, P2P foundations, cache, sync, and storage modules.

Because these components may be used in real applications, security reports are taken seriously.

## Supported Versions

Security fixes are applied to the currently active release branch and the main development branch.

| Version / Branch                                  | Supported                        |
| ------------------------------------------------- | -------------------------------- |
| `main`                                            | Yes                              |
| Active release branches, such as `release/v2.6.3` | Yes                              |
| Older inactive release branches                   | No, unless explicitly maintained |

If you are unsure whether a version is supported, report the issue and include the commit hash or release version.

## Reporting a Vulnerability

Please do **not** open a public GitHub issue for sensitive security vulnerabilities.

Instead, report security issues privately by email:

```txt
adastrablockchain@gmail.com
```

Use a clear subject such as:

```txt
Security report: Vix.cpp <module> vulnerability
```

Example:

```txt
Security report: Vix.cpp HTTP parser vulnerability
```

## What to Include

A good security report should include:

- affected module,
- affected version or commit hash,
- operating system,
- compiler version,
- build configuration,
- clear reproduction steps,
- proof-of-concept input or code if available,
- expected behavior,
- actual behavior,
- potential impact,
- whether the issue is exploitable remotely or locally,
- suggested fix if known.

Example:

```txt
Module: core/session
Version: release/v2.6.3
OS: Ubuntu 24.04
Compiler: GCC 13.3
Build: Release

Issue:
Malformed HTTP request body can trigger unexpected behavior.

Reproduction:
1. Start server with ...
2. Send request ...
3. Observe ...

Impact:
Potential denial of service.

Suggested fix:
Validate content length before ...
```

## Security Scope

Security reports may include, but are not limited to:

- memory corruption,
- use-after-free,
- double-free,
- buffer overflow,
- invalid lifetime handling,
- undefined behavior with security impact,
- data races with security impact,
- denial-of-service vulnerabilities,
- HTTP parser vulnerabilities,
- request smuggling risks,
- WebSocket protocol issues,
- path traversal,
- unsafe file handling,
- unsafe process execution,
- command injection,
- unsafe environment variable handling,
- cryptographic misuse,
- insecure defaults,
- authentication or authorization bypass in modules that provide such behavior,
- P2P message validation issues,
- cache poisoning risks,
- unsafe deserialization or parsing.

## Out of Scope

The following are usually not treated as security vulnerabilities unless they create a real exploit path:

- general code style issues,
- non-security compiler warnings,
- theoretical issues without impact,
- benchmark-only performance differences,
- issues in vendored third-party code without a Vix-specific integration impact,
- unsupported build configurations,
- debug-only behavior not reachable in production builds,
- denial of service caused only by intentionally exhausting local machine resources with no realistic boundary.

If unsure, report it anyway and explain the possible impact.

## Third-Party Dependencies

Vix.cpp may use or integrate with third-party dependencies such as:

- OpenSSL,
- SQLite,
- ZLIB,
- fmt,
- spdlog,
- Asio,
- GoogleTest,
- SDL2,
- MySQL Connector/C++.

If the vulnerability is entirely inside a third-party dependency, please report it to that upstream project.

If the vulnerability is caused by how Vix.cpp uses or configures that dependency, report it to Vix.cpp.

## Response Process

After receiving a security report, the maintainer will try to:

1. acknowledge the report,
2. review the reproduction steps,
3. determine whether the issue is valid,
4. assess severity and impact,
5. prepare a fix if needed,
6. add tests or CI coverage where appropriate,
7. release or merge the fix,
8. credit the reporter if desired.

Response time may vary depending on the complexity of the issue and maintainer availability.

## Disclosure Policy

Please give the project maintainers reasonable time to investigate and fix security issues before public disclosure.

Do not publicly disclose exploit details before a fix is available unless there is a strong public safety reason.

Coordinated disclosure helps protect users.

## Security Fix Requirements

Security fixes should usually include:

- a minimal code fix,
- regression tests,
- sanitizer or Valgrind coverage when relevant,
- static-analysis improvements when relevant,
- documentation or changelog updates if user-facing,
- clear commit messages.

Example commit messages:

```txt
fix: reject malformed HTTP content length
fix: prevent path traversal in static file serving
fix: resolve futures for cancelled queued tasks
test: add regression coverage for invalid request bodies
ci: expand sanitizer coverage for session parsing
```

## CI Security Coverage

Vix.cpp uses CI checks to reduce security and stability regressions.

Security-related CI may include:

- normal module test builds,
- ASan / UBSan sanitizer builds,
- Valgrind memory checks,
- cppcheck,
- clang-tidy,
- package export validation,
- benchmark build validation,
- module-level test coverage.

The security CI is documented in:

```txt
SECURITY_TESTS.md
```

## Safe Testing

When testing security issues:

- do not attack systems you do not own,
- do not run destructive tests against public infrastructure,
- do not exfiltrate data,
- do not publish exploit details before coordination,
- keep proof-of-concept examples minimal and safe.

Local reproductions are preferred.

## Cryptography Notes

Cryptographic code must be handled carefully.

Contributions touching cryptography should avoid:

- inventing custom cryptographic primitives,
- weakening randomness,
- using insecure algorithms by default,
- ignoring authentication tags,
- comparing secrets with non-constant-time comparisons,
- unsafe key or nonce handling,
- logging secrets.

When possible, prefer well-reviewed primitives from established libraries such as OpenSSL.

## Reporting Non-Sensitive Security Hardening

For non-sensitive hardening improvements, public issues and pull requests are welcome.

Examples:

- improving input validation,
- tightening CI coverage,
- adding sanitizer tests,
- improving static-analysis scope,
- documenting secure defaults,
- adding regression tests for previously fixed issues.

## Contact

Security contact:

```txt
adastrablockchain@gmail.com
```

Project:

```txt
Vix.cpp
https://github.com/vixcpp/vix
```

## License

This security policy is part of the Vix.cpp project documentation.

Vix.cpp is released under the MIT License.
