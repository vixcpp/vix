# Vix HTTP Domain — Gated Implementation Plan

Current status (2026-10-19): the experimental, opt-in Linux public slice
provides one-shot TCP client and one-request/one-response server operations.
The separately selected `HTTPSTLSExperimental` package adds authenticated
HTTPS client and server sessions using the existing TLS channel. HTTP-only
consumers do not discover OpenSSL; HTTPS consumers do. Strict Debug, loopback,
installed-consumer, relocation, and focused ASan/UBSan evidence has been
collected for the experimental slice. This is not stable API or
cross-platform qualification. See `HTTP_EXPERIMENTAL.md` and
`HTTP_HTTPS_EXPERIMENTAL.md` for the exact public boundaries.

Historical H0-P4 status: H0-P1 parser/framer, H0-P2B private TCP parent/child loopback proof,
and H0-P3B private authenticated HTTPS exchange proof are implemented behind
the opt-in test-only CMake path. H0-P4B semantic repairs are complete; a
focused H0-P4 security review found and repaired forbidden Content-Length
acceptance on 1xx/204 responses, while full security/package qualification
remains open. At the historical H0-P4 checkpoint there were no public HTTP
headers or stable installed/exported HTTP target. The private HTTPS proof consumes a
moved experimental TlsChannel and supports explicitly framed responses only;
it does not establish a reusable public transport adapter or platform
qualification. Focused Linux strict Debug and ASan/UBSan tests passed locally;
the current security-review validation is summarized under H0-P4 below. These
results are Linux evidence only. Code completion alone is not qualification.

H0-P4-S1 adds an exchange-local maximum of 128 informational responses
(inclusive) and a single absolute 30-second default deadline, shortened by an
earlier caller deadline. Both TCP and HTTPS pass that same boundary through
all child operations and close/retire on timeout. Boundary and timeout tests
are included in the private exchange suites; this does not close the broader
H0-P4 security, packaging, platform, or human-approval gates.

The normative proposal and unresolved questions are recorded in
[HTTP_CONTRACTS.md](HTTP_CONTRACTS.md). The protocol baseline is RFC 9110
(HTTP semantics) and RFC 9112 (HTTP/1.1 syntax and connection management).

## Global implementation rules

- Keep HTTP protocol machinery in one shared HTTP/1.1 engine. Client and
  server are different roles around the same parser/framer, not parallel
  protocol implementations.
- Network owns sockets, provider progress, native readiness, cancellation and
  deadlines at the transport-operation boundary. HTTP must not create a
  reactor, worker pool, scheduler, global runtime, or raw socket path.
- Reuse Standard `Result`, `Error`, `Completion`, `CancellationToken`, and
  `Deadline`. Do not add an HTTP future/task type unless Network's private
  composition seam proves no smaller path can supply correct operations.
- Keep memory finite. Streaming means bounded staged bytes and backpressure,
  not an implicit unbounded body buffer.
- A partially transmitted or ambiguously framed message is not assumed
  reusable. Child operations and borrowed storage retire before parent
  terminal completion.
- HTTP/1.1 plaintext support must not depend on OpenSSL. HTTPS is explicit,
  optional, and unavailable unless TLS is independently qualified. Never
  downgrade an HTTPS request to plaintext.
- Do not modify Vix2 or expose backend, parser-state, reactor, or transport
  adapter mechanics as public API without a separately approved need.

## H0-P1 — Parser and framing feasibility proof

### Scope

Implement a private, incremental HTTP/1.x parser and serializer with no socket
or third-party HTTP dependency. Exercise it with byte spans supplied in
arbitrary splits and coalescing patterns. Establish message framing and error
policies before client/server APIs are proposed for freeze. (The implementation
accepts HTTP/1.0 and HTTP/1.1 syntax; HTTP/1.1 remains the proposed initial
network capability.)

### Dependencies

- RFC 9110 semantics and RFC 9112 syntax, framing, and connection rules.
- Standard error identity conventions, but no Network or TLS integration yet.

### Exclusions

- No public headers/target, TLS, DNS, sockets, client/server facade, body
  streaming over a live connection, HTTP/2/3, proxy, WebSocket, routing, or
  application conveniences.
- No permissive parser behavior chosen for compatibility unless the RFC
  requires it and a single interpretation is demonstrated.

### Acceptance tests

- Request and response start-lines with valid and invalid versions, methods,
  targets, reason phrases, and status codes as applicable.
- Header field token/value validation, CRLF handling, obs-fold policy,
  whitespace, line/header count/total-size limits, and byte-oriented parsing.
- Content-Length: absent, zero, valid, overflow, invalid syntax, duplicate
  equal and conflicting values, and comma-list cases as specified.
- Transfer-Encoding: chunked, valid coding order, malformed coding, duplicate
  or unsupported codings, chunk-size overflow, extensions, data terminators,
  trailers, premature EOF, and forbidden trailer fields.
- Every Transfer-Encoding/Content-Length combination, especially ambiguous
  combinations. Verify one exact RFC-defined decision or rejection plus
  connection closure; never allow a second interpretation.
- No-body semantics for applicable request/response cases and close-delimited
  responses only where allowed.
- Delimiter splitting at every octet boundary; multiple messages in one input
  span; incomplete message continuation; exact surplus-byte retention.
- Resource limits for start-line, fields, chunk metadata, and internal
  buffers; deterministic failure without unbounded allocation.
- Serialization round trips preserve valid semantics and cannot emit
  contradictory framing fields.

### Current implementation and verification status

- Private source: `src/Http1.hpp`, `src/Http1.cpp`.
- Private table-driven corpus: `tests/http1_protocol_test.cpp`.
- Opt-in target wiring: `CMakeLists.txt` and `tests/CMakeLists.txt`; root
  option `VIX_BUILD_HTTP_P1_TESTS` defaults OFF. The test executable compiles
  the parser source directly and does not install/export a library or link
  Network, TLS, OpenSSL, or Standard.
- Current bounded defaults: 8 KiB start line, 32 KiB/100 header fields,
  4 KiB chunk line, 64 KiB chunk metadata, 8 KiB/32 trailer fields, and 64 KiB
  staged input. Body bytes stream to caller-provided output and pause without
  consuming data when that output is empty. Serializer output lengths are
  preflighted against finite start-line/header/trailer bounds before output
  allocation; invalid serialization preserves the prior output.
- Fresh GNU 13.3.0 strict-warning Debug build and focused CTest passed from
  `/tmp/vix-http-p1-debug`; the same test passed in fresh Release and explicit
  ASan+UBSan builds. Sanitizer compile commands and `link.txt` contained
  `-fsanitize=address,undefined`. LeakSanitizer itself failed in this
  container with its unsupported-under-ptrace diagnostic; rerunning with
  `ASAN_OPTIONS=detect_leaks=0` passed under ASan+UBSan. This does not qualify
  leak behavior.
- Release `flags.make` contains `-DNDEBUG -UNDEBUG`; the test uses an explicit
  aborting `CHECK` macro, so its expectations remain active in Release.
- A fresh HTTP-option-OFF configure exposed no HTTP P1 target in generated
  build target help. The option adds only the private test executable and has
  no install/export rule.
- Not established: cross-platform behavior, exhaustive RFC conformance,
  fuzz/sanitizer qualification, live Network integration, or public limit/API
  suitability.

### CI exit criteria

- Table-driven parser/framer tests pass under strict warnings and sanitizers
  on the primary native toolchain.
- Fuzz/property testing reports no crash, out-of-bounds access, overflow,
  unbounded growth, or inconsistent framing decision for the tested corpus.
- A written framing decision table maps each accepted/rejected case to RFC
  9110/9112 sections and the expected connection disposition.
- Human review approves anti-smuggling behavior and safe error recovery before
  any network-facing code is added.

## H0-P2 — Network operation composition feasibility

### Scope

Prove the narrowest way for an HTTP-owned parent operation to compose
Network-owned child operations and resume through the same provider, in both
background and cooperative modes. Prove HTTP-internal byte-stream adapters
for plain `TcpConnection` and (only in an experimental integration) `TlsChannel`.

### Dependencies

- P1 parser/framer decisions.
- Existing `network::Operation<T>`, `OperationOptions`, provider lifetime,
  child awaiter/borrowed-buffer rules, and private `NetworkAccess` patterns.
- TLS public channel remains experimental/uninstalled in current source
  (`domains/tls/TLS_CONTRACTS.md`).

### Exclusions

- No public generic byte-stream hierarchy, raw descriptor access, second
  progress engine, or public HTTP/TLS package dependency.
- No claiming that direct access to `domains/network/src/Engine.hpp` is a
  durable domain-integration contract merely because TLS currently uses it.

### H0-P2B private proof implemented

- `src/PrivateExchange.cpp` claims the HTTP-specific private Network lease,
  creates one parent `network::Operation<ExchangeResponse>` with the existing
  private `OperationFactory`, owns all borrowed storage in the parent state,
  and awaits send/receive children without destroying an incomplete child.
- The same `Http1Parser` used by P1 parses a real loopback request and response;
  response reads are intentionally fragmented into 11-byte Network receive
  buffers. A private Network test hook caps native sends at three bytes for
  the successful exchange and asserts multiple native send/receive attempts.
  Fixed-length truncation, pre-start and in-flight cancellation and deadline
  paths, exactly-once publication, provider resource retirement, and
  cooperative foreign-thread rejection are covered.
- After child send completion the parent yields its next receive start through
  the existing provider command queue. This handles the observed continuation
  versus stale-readiness/descriptor-arm rendezvous without adding a runtime.
  Network's private operation state also fixes the observed completion-before-
  suspend publication race; its deterministic regression covers both terminal
  publication winners. The handoff awaiter does not access frame state after
  publishing its continuation, preventing a resume-before-return race.
- Local results: strict GCC 13.3 Debug focused Network/HTTP/TLS suite 14/14;
  ASan/UBSan focused suite 5/5; private exchange repeated 10/10 in strict
  Debug and 5/5 under ASan/UBSan. Results are Linux-only.
- This remains a test-only composition proof. It does not create a public
  Client/Server, reusable connection/session, body streaming API, or installed
  target. At that phase, TLS composition was not yet implemented; see P3B.

### H0-P3B private HTTPS proof implemented

- `src/PrivateTlsExchange.cpp` creates a Network-provider-bound parent
  `Operation<ExchangeResponse>`, consumes a moved `TlsChannel`, completes its
  authenticated handshake before sending the serialized HTTP request, and
  reuses the P1 response parser for incrementally decrypted application data.
  TLS retains exclusive ownership of the underlying TCP lease; HTTP never
  claims or accesses the socket. The parent owns request, parser, receive and
  body storage, awaits each TLS child to terminal completion, performs mutual
  TLS shutdown on success, closes the channel on terminal paths, and publishes
  one parent completion.
- The private build-only `HttpTlsAccess` bridge obtains the provider already
  retained by the TLS channel; it exposes no socket, lease, or public stream
  interface. The HTTPS test target exists only when both private HTTP tests and
  `VIX_BUILD_TLS_EXPERIMENTAL` are enabled; it is not installed/exported.
- A real loopback test uses a local explicitly trusted test certificate,
  verifies DNS identity mismatch rejection before HTTP success, constrains
  native sends and receive chunks to exercise partial progress, checks TLS
  ownership exclusion, fixed-length HTTP framing across TLS reads, cancellation
  and deadlines, provider retirement, and mutual close_notify. It also verifies
  that fixed-length truncation remains an HTTP failure and that close-delimited
  responses are rejected by this private exchange rather than inferred from
  TLS close_notify.
- Validation on Linux: strict GCC Debug build and complete 37/37 CTest suite
  passed; ASan/UBSan instrumentation was confirmed in compile and link commands
  and the six focused Network/HTTP/TLS tests passed 6/6; the HTTPS test passed
  five consecutive bounded repetitions. Canonical architecture target built.
  This is Linux evidence only, not platform or stable TLS qualification.
- No public HTTP API, stable TLS dependency, reusable connection/session,
  streaming body API, or installed SDK capability is established.

### Acceptance tests

- Parent HTTP-like operation starts Network children, retains every started
  incomplete child and borrowed buffer, and publishes parent Completion only
  after terminal child retirement.
- Cancellation/deadline races with receive/send/native success exactly once;
  partial send invalidates reuse unless protocol alignment is proven.
- Background provider progresses without blocking its continuation workers.
- Cooperative provider has no progress without owner-thread `poll()` and
  progresses with polling; no helper thread is created.
- Multiple child reads/writes preserve byte order and never overlap forbidden
  Network direction slots.
- Provider shutdown remains rejected while the HTTP parent/session owns
  transport resources and succeeds after safe retirement.
- HTTPS adapter is absent when experimental TLS is disabled; selecting HTTPS
  in that build returns explicit unsupported capability, never plaintext.
- Build-tree and installed-consumer experiments determine the minimal private
  operation factory/access boundary and whether it survives separate domain
  compilation without exporting internals.

### CI exit criteria

- One reviewed, private operation-composition seam is demonstrated or the
  blocker and smallest required Network change are documented.
- No HTTP progress thread/reactor is created; provider and borrowed-lifetime
  contracts hold under sanitizers.
- Plain HTTP's link graph contains no TLS/OpenSSL dependency.
- No public API is frozen until cooperative/background and lifetime tests pass.

## H0-P3 — Minimal HTTP client and server exchange

### Scope

Add a small public HTTP/1.1 client exchange and server request/response
capability over Network using the single P1 engine and P2 composition path.
Expose request/response fields, bounded configuration, and streamed body
source/sink behavior only as necessary for real message exchange.

### Dependencies

- P1 protocol/framing review approval.
- P2 operation-composition and adapter proof.
- Standard and Network targets; no TLS dependency for plaintext HTTP.

### Exclusions

- No automatic redirects, retries, cookie jar, cache, proxy, connection pool,
  pipelining by default, router, middleware, handler framework, JSON,
  compression, templates, or application server runtime.
- No HTTPS claim before P4 and separate TLS qualification.

### Acceptance tests

- GET and POST request serialization against local loopback fixtures; verify
  method/target/field semantics and exact body framing.
- Server parses a request and writes a response without imposing dispatch or
  routing policy.
- Large fixed-length and chunked bodies stream with bounded memory, producer
  and consumer backpressure, and exact order.
- Partial reads, coalesced messages, persistent-connection state, EOF, and
  keep-alive behavior follow the approved framing state machine.
- Cancellation/deadline before connect, during headers, body receive/send,
  and between messages produces one completion; borrowed buffers and child
  awaiters remain alive to retirement.
- Malformed input and framing conflict close or recover exactly as P1
  specified. A client/server pair cannot disagree on request boundaries.
- Cooperative and background Network tests pass, including no-poll behavior.
- Provider shutdown/accounting, connection close, response truncation, and
  resource exhaustion have deterministic tests.
- Installed SDK consumer uses plaintext HTTP without resolving OpenSSL.

### CI exit criteria

- Full HTTP suite passes on each claimed Network platform; cross-platform
  qualification includes supported Windows/macOS/Linux variants and any
  supported Android environment.
- Strict zero-warning build, sanitizers on supported toolchains, installed
  package consumer and public compile-contract tests pass.
- Unsupported Network targets are explicitly unavailable; no silent fallback.

## H0-P4 — Security, optional TLS composition, and package qualification

### Focused H0-P4B correctness subtask

H0-P4B is a correctness gate within this existing milestone; it does not
replace or rename H0-P4. Before any broader security/package qualification,
the private parser and exchange tests must cover informational-to-final
response selection, explicit rejection of unsupported 101/CONNECT transitions,
request-method-aware response framing, version-independent validation of any
present Host, 205 zero-content semantics, and absolute-form authority/Host
consistency. The TCP and experimental HTTPS exchanges must share these
framing decisions. Close-delimited HTTPS responses remain unsupported.

The focused gate requires strict Debug tests for the parser/serializer and
both private exchange paths in background and cooperative Network modes, plus
the existing Network/TLS lifecycle regressions. Sanitizer and repeated-loopback
results must be reported separately from cross-platform qualification. No
public API, installed target, protocol expansion, or application framework is
part of this subtask.

### H0-P4 security review status — 2026-10-19

The original threat checklist is dispositioned in the “H0-P4 security threat
checklist disposition” section of `HTTP_CONTRACTS.md`; the P4A audit remains
historical and was not replaced. Source review found that the parser accepted
Content-Length on 1xx and 204 responses despite the RFC 9110 §8.6 prohibition.
The parser now rejects presence of that field, including `Content-Length: 0`,
with `invalid_response_semantics`. HEAD/304 representation metadata and the
successful-CONNECT exception are unchanged. H0-P4-S1 then added a cumulative
128-response limit and one absolute 30-second default exchange deadline
(shortened by an earlier caller deadline) to both private TCP and HTTPS paths.
The 128th informational response is allowed; the 129th fails with the
resource-limit identity and retires the exchange. The deadline spans handshake,
request send, response receive, and successful TLS shutdown; deadline cleanup
closes rather than starting another graceful exchange. These are private
prototype policies, not a process-wide quota or hard real-time promise.

The intermittent HTTPS stall was separately reproduced and diagnosed from
thread stacks as a Network startup lock inversion: a caller held the TCP
connection mutex while synchronously waiting for provider-thread descriptor
registration, while provider progression needed that mutex. TCP receive/send
starters now queue descriptor arming and initial progression through the
provider; the provider thread performs the arm and checks that the operation
is still active before native I/O. This preserves Network ownership and
cooperative polling, but remains Linux-tested rather than cross-platform
qualified. No persistence or resynchronization is supported. Stable TLS
packaging, installed HTTP/HTTPS consumer tests, capability behavior,
cross-platform conformance, and human approval remain outstanding. Do not
advertise or install HTTP/TLS based on this private review.

Latest Linux/GCC 13.3 evidence (2026-10-19): strict Debug build of affected
targets and `vix_canonical_architecture` passed; the full configured CTest
suite passed 37/37; the complete HTTPS matrix passed once and three additional
consecutive times; focused ASan/UBSan Network lease, TLS private driver,
plaintext exchange, and HTTPS exchange tests passed 4/4. Compile commands and
executable link files contained `-fsanitize=address,undefined`; this sanitizer
build used explicit compiler/linker flags because `VIX_ENABLE_SANITIZERS=ON`
alone does not instrument these Domain targets. No explicit `ASAN_OPTIONS`
override was set for the latest focused run; LeakSanitizer was not separately
qualified, and no ThreadSanitizer, external fuzzing, or non-Linux runtime
qualification was performed. Loopback tests require running outside the
restricted socket sandbox; a sandboxed socket send returned `EPERM`, while the
same focused suite passed with socket permission.

### Scope

The private HTTP protocol-security subset now has bounded per-message and
per-exchange resource behavior plus Linux regression evidence. The broader
H0-P4 milestone remains open: qualify HTTPS integration only if TLS becomes
independently supported and installed; complete capability reporting, stable
SDK packaging, documentation, and platform conformance for the HTTP contract.

### Dependencies

- P1–P3 accepted and tested.
- TLS independently passes its own backend/authentication/transport/runtime
  gates, and a stable TLS installed target exists.
- Explicit dependency, trust, identity, and unsupported-target decisions.

### Exclusions

- No weakening TLS verification for convenient tests, HTTP downgrade,
  app-level authentication system, proxy tunnel support, or protocol upgrades
  without an approved later scope.

### Acceptance tests

- Trusted and untrusted HTTPS peers, expected DNS/IP identity match/mismatch,
  SNI behavior, cancellation during TLS handshake, and no application bytes
  before authenticated TLS success.
- TLS unavailable/disabled: HTTPS fails with a stable capability error and
  never retries as `http`.
- Plain HTTP installed consumer and binary remain OpenSSL-free when optional
  TLS integration is not selected.
- Installed HTTPS consumer links only through explicit qualified TLS target;
  no backend-private headers, build paths, or accidental transitive dependency
  leak.
- Security review covers request smuggling, response splitting, field
  injection, truncated bodies, persistent connection resynchronization,
  resource exhaustion, and error-path resource ownership.
- Linux glibc/musl, macOS, Windows, and Android qualification as available;
  WASI Preview 2 is not claimed unless Network and HTTP runtime requirements
  are independently proven.

### CI exit criteria

- Every platform is labeled separately as API/build, runtime, and full
  conformance verified. Source presence or cross-compilation alone is not
  runtime evidence.
- No release capability is advertised without installed-consumer verification
  and the complete applicable parser, transport, security, and lifecycle
  suite.
- Human approval of HTTP error identities, limits, connection reuse defaults,
  public body API, and optional TLS packaging is required before stable API
  freeze.

## Suggested smallest next step

Review the P1 framing policy and the private P2B composition evidence. If
approved, the next step is a separate H0-P3 API design/feasibility decision;
do not promote these private test types into a public API or treat the local
Linux results as cross-platform qualification.

This recommendation is historical and was superseded by HTTP-PUBLIC-1.

## HTTP-PUBLIC-1 experimental package status

The current package implementation is separate from the historical H0 roadmap
above. It builds only when `VIX_BUILD_HTTP_EXPERIMENTAL=ON`, installs/exports
only when `VIX_INSTALL_HTTP_EXPERIMENTAL=ON`, and is limited to Linux. The
default SDK remains unchanged. `exchange()` and `accept()` synchronously
validate/copy bounded request data and return `Result<network::Operation<T>>`;
`network::run` then supplies a blocking `Result<Completion<T>>` entry point.
Client and server close each connection after one exchange. Incoming trailers
are preserved in message values; outgoing trailers and caller-controlled
framing/Connection headers are rejected, while Vix generates fixed-length
framing and `Connection: close`.

This slice does not close the original broader H0 security, TLS, platform,
stable API, or human-approval gates. Cooperative `network::run` is owner-thread
blocking and progresses only through the existing provider wait/poll path; the
previously documented fatal recovery behavior for a native cooperative reactor
wait failure remains unqualified and was not changed here.
