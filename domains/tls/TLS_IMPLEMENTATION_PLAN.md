# Vix TLS Domain — Implementation Plan

Status: gated phase plan. The opt-in experimental OpenSSL 3 target includes
client/server configuration, channel creation, authenticated handshake,
secure receive/send, abortive close, graceful close_notify exchange, and
private Network-backed tests. A separate opt-in Linux package component is now
installable/exportable for consumer qualification; it remains provisional and
is excluded from default Vix installations. Linux runtime evidence exists,
but I0/I1/I2 acceptance criteria and release qualification remain open; no
platform is production-qualified.

The implementation sequence is gated: a phase does not advance because code
compiles alone. It advances only when its stated behavioral evidence and
platform claims are satisfied. Contracts and unresolved choices are in
[TLS_CONTRACTS.md](TLS_CONTRACTS.md).

## Global rules

- TLS is a standalone foundational Domain, independent of HTTP and application
  protocols. It composes with Standard and Network and does not require
  Concurrency or a global runtime.
- Standard remains the sole owner of `Result`, `Error`, `Completion`,
  `CancellationToken`, and `Deadline` semantics.
- Network remains the sole owner of sockets, readiness, native I/O progression,
  cooperative polling, and provider lifecycle. TLS must not access native
  descriptors directly.
- All backend calls made on the Network progress path must be nonblocking.
  Backend access is serialized per TLS session. One logical read and one
  logical write may coexist only through the TLS protocol driver.
- Unsupported platforms fail explicitly; there is no plaintext fallback.
- Do not publish a public header or package target until its contract is
  implementable and testable on the proposed support set.
- No phase modifies legacy Vix2 or introduces HTTP, a TLS-owned reactor, worker
  pool, scheduler, or second cancellation/deadline system.

## Current experimental implementation inventory

- `TlsClientConfig`/`TlsServerConfig` own an OpenSSL context; the client
  requires a DNS/IP reference identity and trusted roots. DNS SNI defaults to
  that identity but can be overridden independently.
- `TlsChannel::create_client`/`create_server` construct the engine before
  atomically claiming the Network connection, so ordinary synchronous setup
  failures do not consume the connection. The channel exposes handshake,
  receive, send, abortive close, and graceful shutdown operations.
- The OpenSSL memory-BIO driver is bounded at 32 KiB per BIO and retains
  stable write storage across retry. PrivateChannel retains child Network
  awaiters and staging buffers until each child completes.
- The experimental CMake target is opt-in and Linux-only. Installation/export
  is separately gated by `VIX_INSTALL_TLS_EXPERIMENTAL`; the exported
  `TLSExperimental` package component resolves OpenSSL only when explicitly
  requested. The private driver test does not constitute the complete
  security/platform acceptance matrix below.

## TLS-I0 — Backend and Network integration feasibility

### Scope

The current private prototype is intended to provide executable feasibility
evidence before public API or backend commitment. It exercises serialized
OpenSSL session access, bounded BIOs, Network-owned handshake and stream I/O,
concurrent logical read/write composition, and parent/child lifetime retention.
These source paths are not executable proof yet; the full retry/cancellation/
deadline matrix, authentication policy, and platform support remain
unverified. Continue to require:

1. Compare maintained TLS backend candidates. Evaluate licensing, maintenance,
   supported targets/toolchains, C++ integration, nonblocking memory-BIO or
   equivalent controllable I/O, bounded allocation, certificate-chain and
   reference-identity verification, SNI, ALPN, diagnostics, entropy needs,
   exception-disabled builds, and installed SDK dependencies. Document why a
   candidate meets or fails each required property. Do not integrate a backend
   merely because it is already present in an unrelated module.
2. Prototype privately whether Network can provide a narrow internal
   `TcpConnection` lease. Prove atomic claim against active operations,
   dormant operations, concurrent starters, in-flight native I/O, cancellation
   callback retirement, provider shutdown, failed claim, and destruction.
   Public Network API changes are not authorized by this plan.
3. Demonstrate a serialized, nonblocking TLS session driver using Network
   child operations. Exercise a backend requiring read progress during write
   and write progress during read. Prove no overlapping backend entry, no
   lost wakeup, and no lock held across suspension.
4. Demonstrate the same design with background and cooperative Network
   providers. Cooperative mode must make progress only via owner-thread poll;
   background mode must not block a reactor/continuation worker or create TLS
   threads.
5. Model cancellation/deadline ownership for handshake, partial receive,
   buffered plaintext, generated ciphertext, partial send, timeout, native
   success races, and shutdown. Prove child operations and borrowed spans stay
   alive through terminal completion.
6. Prove client trust-root and reference-identity behavior, server identity
   handling, and entropy availability on each candidate platform. Select
   explicit secure defaults; reject configurations that would silently disable
   peer verification.
7. Publish a backend/platform capability matrix separating source
   availability, cross-build, runtime execution, and conformance qualification.

### Dependencies

- Existing Standard and Network contracts; no dependency on Concurrency.
- Maintained backend candidate source/documentation and reproducible toolchain
  metadata for evaluation only. No vendoring/integration before approval.
- A private Network test seam or prototype may be required to exercise claim
  races; keep it internal and do not alter the public API during feasibility
  work.

### Exclusions

- No public TLS headers, CMake target, SDK export, stable error catalog, or
  installed package dependency.
- No production backend selection, trust-store policy, or platform support
  claim.
- No HTTP/ALPN application framework, session cache, client pool, proxy, or
  certificate management service.
- No broad Network redesign and no native-thread emulation on WASI.

### Acceptance tests / evidence

- A written candidate comparison with licensing and supply-chain provenance.
- Deterministic transport-claim race harness: active read/write reject; claim
  versus starter has one winner; dormant pre-created operations cannot bypass
  a winning claim; failed claim leaves TCP usable.
- Instrumented/mock backend confirms only one TLS engine call is active and
  supports both WANT_READ-during-write and WANT_WRITE-during-read progress.
- Loopback prototype passes in background and cooperative modes; cooperative
  no-poll check proves no autonomous progress and no hidden worker exists.
- Fault injection covers partial native send, cancellation callback, deadline,
  provider shutdown, buffer exhaustion, backend allocation/error paths, and
  authentication failure while preserving child/buffer lifetime.
- Client identity tests distinguish SNI from reference identity and reject
  untrusted/mismatched peers. Entropy and root provisioning are verified from
  the actual backend/toolchain on every proposed target.

### CI exit criteria

- No platform is marked supported based only on source compilation.
- Linux primary-host prototype and at least one second native OS feasibility
  run are required before selecting a backend for I1; exact runner matrix is
  approved with the candidate.
- Background and cooperative progress tests pass without TLS-created threads.
- All seven feasibility gates above have evidence and human review approval.
- If any gate fails, revise the design or narrow the proposed platform set;
  do not proceed by weakening authentication, ownership, cancellation, or
  lifetime requirements.

## TLS-I1 — Authenticated client/server handshake

### Scope

After I0 approval, add the minimum public Domain identity and package target,
private Network lease integration, backend adapter, security configuration,
and client/server handshake. Publish a usable secure channel only after the
configured peer-authentication policy succeeds. Use Standard synchronous
`Result` for construction/claim failures and `Completion` for asynchronous
handshake outcome. Expose machine-readable TLS error identities with backend
diagnostics as supplemental detail.

### Dependencies

- Approved I0 backend and support matrix.
- Proven private Network lease and serialized driver design.
- Approved trust-root, reference-identity, SNI, server identity, client-auth,
  protocol-version, ALPN, buffer-bound, and error policies.
- Standard and Network public installed targets only; no Concurrency runtime
  requirement.

### Exclusions

- No secure stream read/write API beyond what handshake construction requires.
- No HTTP, automatic redirects, connection pooling, session cache, certificate
  issuance, or silent insecure mode.
- No TLS support for a target lacking I0 capability and runtime proof.

### Acceptance tests

- Client authenticates a trusted matching DNS/IP peer; rejects untrusted chain
  and mismatched reference identity; verifies SNI is not identity validation.
- Server presents configured identity; optional mutual TLS rejects absent or
  invalid client certificate when policy requires it.
- No application plaintext can be read/written/published before authenticated
  handshake terminal success.
- Handshake failure, cancellation, deadline, backend error, and transport
  failure each produce one terminal completion and safely retire/close the
  transport.
- Active/dormant TCP operation lease tests and provider lifetime tests pass.
- Background and cooperative provider tests verify correct progress ownership.

### CI exit criteria

- Strict zero-warning build and all TLS-I1 tests pass on each declared target.
- Installed SDK consumer links through public Vix targets without private
  include paths.
- Client and server authentication negative tests are mandatory, not skipped.
- No release artifact advertises TLS on an unqualified platform.

## TLS-I2 — Secure stream operations and lifecycle

### Scope

Add bounded, backpressured TLS read/write operations, one logical operation per
direction, serialized backend progression, cancellation/deadline integration,
and explicit channel shutdown/close behavior. Distinguish clean TLS
`close_notify` from truncated TCP EOF. Keep each TLS parent and Network child
state alive until exactly-once terminal publication.

### Dependencies

- TLS-I1 authenticated channel contract and backend adapter.
- I0 proof of nonblocking driver, bounded buffers, and background/cooperative
  scheduling behavior.
- Approved write cancellation policy (recommended: close after interrupted
  committed ciphertext send) and read-state recovery rule.
- Approved semantics for local close-notify completion versus waiting for peer
  close-notify.

### Exclusions

- No generic async operation framework, Network reactor replacement, hidden
  worker, automatic retry of application writes, or cross-channel pooling.
- No promise that successful send implies peer receipt or application
  consumption.
- No reuse after uncertain partial ciphertext transmission or truncation.

### Acceptance tests

- Concurrent read/write with backend WANT_READ/WANT_WRITE interleavings proves
  serialized backend calls, no deadlock, exactly-once parent completion, and
  correct byte ordering.
- Deterministic backpressure and each configured buffer limit prove bounded
  memory, correct resource-exhaustion result, and no silent byte loss.
- Cancellation/deadline matrix covers pre-I/O cancellation, partial record
  receive, retained plaintext, generated ciphertext, partial send, native
  success races, and shutdown; verify reuse or mandatory closure as specified.
- Clean close-notify and raw EOF produce distinct outcomes; local shutdown
  waits until close-notify bytes have been accepted before TCP write-half
  shutdown.
- Sanitizers/lifecycle tests prove borrowed caller buffers, backend buffers,
  awaiters, and child Network operations outlive all possible accesses.
- Cooperative tests require explicit `poll()`; background tests confirm TLS
  never monopolizes Network's reactor or required continuation workers.

### CI exit criteria

- All I1/I2 operation, race, cancellation, deadline, truncation, and shutdown
  tests pass on each target declared supported.
- No test turns authentication, close-notify, or capability failures into
  unconditional skips.
- Buffer bounds are checked under stress and sanitizer instrumentation.
- Provider shutdown cannot report quiescence while a TLS lease or child
  operation remains live.

## TLS-I3 — Security, installed SDK, and platform qualification

### Scope

Complete security and release qualification: backend update/provenance policy,
trust-root lifecycle, identity/hostname edge cases, protocol version/cipher
policy review, secure diagnostics, installed SDK and relocation checks,
platform capability declaration, and release documentation. Validate the
actual package contents and downstream use, not only source-tree tests.

### Dependencies

- TLS-I1 and TLS-I2 contracts and tests complete.
- Backend licensing, update, vulnerability-response, entropy, and root-store
  policy approved.
- Reproducible platform runners, test certificates/keys isolated from
  production trust/signing material, and supported runtime environments.

### Exclusions

- No production certificate authority service, key-management product,
  application framework, HTTP policy, or unsupported platform fallback.
- No claim of FIPS or other formal certification without separate evidence.

### Acceptance tests

- Security review covers authentication defaults, hostname/IP verification,
  malformed chains, expiration, key usage, protocol downgrade, ALPN behavior,
  close-notify/truncation, secret handling, and error redaction.
- Installed SDK consumer configures, links, runs, and relocates without build
  tree/backend-private paths. Backend shared/static dependencies and notices
  match the actual package contract.
- Native client/server conformance runs on each declared host/target, including
  Linux glibc and musl where claimed, macOS Intel/Apple Silicon, Windows
  architectures claimed, Android ABIs claimed, and WASI only if separately
  approved and supported.
- Cross-compilation, emulation, and native runtime evidence are recorded as
  separate classes. Unsupported platforms return the explicit capability
  error.
- Strict warnings, required tests, sanitizers where available, dependency
  integrity, and package/license checks pass without suppressing applicable
  coverage.

### CI exit criteria

- Security and platform owners approve the backend and trust policy.
- Every published platform has native runtime qualification for the contracts
  it advertises; cross-build alone is insufficient.
- Installed-package and relocation consumers pass on every release artifact
  class.
- Release metadata identifies exact backend/dependency versions and supported
  TLS capabilities. No unqualified target is advertised as TLS-capable.
- TLS release gate is mandatory; any required security/platform job failure
  prevents publication.

## Phase-transition rule

TLS-I0 is an evidence and decision phase, not permission to begin implementation
or to commit to a backend. Human approval is required after its candidate,
lease, progress, authentication, and platform evidence is reviewed. Only then
may TLS-I1 implementation be explicitly authorized. Subsequent phases likewise
require review of their acceptance evidence; a green compile alone does not
advance the security contract.
