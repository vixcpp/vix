# Vix TLS Domain — Normative Contract Proposal

Status: proposed contracts plus an opt-in, uninstalled experimental OpenSSL 3
implementation. Build-tree public headers now expose client/server
configuration, authenticated handshake, secure stream operations, abortive
close, and graceful shutdown. The implementation and tests have not been
compiled or runtime-validated in this task; this does not establish any
qualified platform claim or a stable installed SDK contract.

## 1. Contract status and boundaries

### Established Vix contracts

- `vix::Result<T>` represents synchronous success or failure; `vix::Completion<T>`
  represents one terminal asynchronous outcome (`Success`, `Failure`, or
  `Cancelled`). The provider, not `Completion`, arbitrates completion races
  ([Completion.hpp](../../standard/include/vix/Completion.hpp:13),
  [Result.hpp](../../standard/include/vix/Result.hpp:1)).
- `vix::Error` carries a machine-readable `ErrorIdentity`; diagnostics are not
  the identity ([Error.hpp](../../standard/include/vix/Error.hpp:36),
  [ErrorIdentity.hpp](../../standard/include/vix/ErrorIdentity.hpp:44)).
- `vix::CancellationToken` callbacks run synchronously on the winning
  cancellation thread. Registration after cancellation may invoke the callback
  inline. Callbacks must not be invoked while holding locks they may need
  ([Cancellation.hpp](../../standard/include/vix/Cancellation.hpp:257)).
- `vix::Deadline` is an absolute monotonic boundary. It does not itself encode
  timeout completion; absence is represented by `std::optional<Deadline>`
  ([Deadline.hpp](../../standard/include/vix/Deadline.hpp:12)).
- Network `Operation<T>` is a move-only coroutine awaitable. Its started
  incomplete awaiter must not be destroyed; doing so is a contract violation.
  It uses Standard cancellation and deadlines
  ([Operation.hpp](../network/include/vix/network/Operation.hpp:14),
  [Operation.hpp](../network/include/vix/network/Operation.hpp:57)).
- `TcpConnection` is move-only. Its receive/send spans are borrowed through
  terminal completion. A successful send means the complete span was accepted
  by the socket; a failed send may already have transmitted a prefix
  ([TcpConnection.hpp](../network/include/vix/network/TcpConnection.hpp:12)).
- Network owns transport readiness and progression. Background providers own
  their reactor/continuation machinery; cooperative providers progress only
  when their owner calls `Provider::poll()` and remain thread-affine
  ([COOPERATIVE_PROGRESS.md](../network/COOPERATIVE_PROGRESS.md:1)).

### Proposed TLS normative contracts

TLS is a separate foundational Domain for secure stream channels. It is
independent of HTTP, application protocols, and any Vix application runtime.
It reuses Standard's error, completion, cancellation, and deadline semantics;
it does not define replacement types. Network remains the sole owner of native
transport I/O, readiness, provider progress, and provider lifecycle. TLS owns
the cryptographic session, authentication policy, TLS protocol state, bounded
TLS buffers, and serialization of access to its backend session.

TLS must use a maintained cryptographic/TLS implementation. Vix must not
implement cryptographic primitives or a TLS protocol engine. Backend selection,
trust-store integration, entropy guarantees, and platform support remain
unresolved until TLS-I0 candidate evaluation and executable feasibility proof.

### Awaiting implementation feasibility evidence

The following are design requirements, not established repository facts:

1. A private Network transport lease can atomically exclude ordinary TCP
   operations and hand exclusive transport access to TLS.
2. A selected backend can be driven nonblockingly with bounded and controllable
   input/output buffers, without internal worker creation or blocking calls.
3. A TLS driver can serialize all backend session access while allowing one
   logical read and one logical write to make progress.
4. The driver can preserve Network's background and cooperative progress
   contracts without blocking a reactor or continuation worker.
5. Authentication, trust-root provisioning, peer reference identity, entropy,
   close-notify, truncation, cancellation, and deadline behavior can be tested
   with the selected backend on every advertised target.

### Explicitly unsupported or not yet promised

- The build-tree TLS API and OpenSSL 3 backend are experimental and gated by
  `VIX_BUILD_TLS_EXPERIMENTAL`; the target is not installed/exported. A CMake
  platform branch or available system library is not qualification evidence.
- WASI Preview 2 native TLS execution is unsupported unless TLS-I0 proves a
  maintained backend, suitable entropy/trust facilities, and a cooperative
  nonblocking integration. Network's source-level WASI cooperative provider is
  itself awaiting runtime conformance evidence
  ([WASI_P2_INTEGRATION.md](../network/WASI_P2_INTEGRATION.md:1)).
- No TLS-owned reactor, execution pool, scheduler, global runtime, cancellation
  model, or deadline model is permitted. TLS does not require Concurrency.
- No unauthenticated application plaintext is exposed to callers before the
  configured authentication policy succeeds.
- No guarantee of hard real-time deadline response, forced interruption, or
  delivery to the peer/application is made.

## Experimental implementation behavior (not yet qualified)

- `TlsClientConfig` requires an explicit DNS/IP reference identity and at
  least one trust source. Verification is enabled; system roots are used by
  default, and an optional CA file can add roots. Supplying a CA file with
  system roots disabled is the explicit private-root-only configuration.
- DNS reference identity is verified independently from SNI. If no explicit
  SNI name is supplied, a DNS reference identity is also used as SNI; IP
  identities do not automatically produce SNI. No API disables peer
  verification.
- `TlsServerConfig` requires a certificate chain and matching PEM private key.
  Client-certificate authentication is not exposed in this experimental API.
  TLS 1.2–1.3 version bounds are configured explicitly.
- The experimental secure stream has one active logical receive and one active
  logical send. OpenSSL entry is mutex-serialized and Network child operations
  are awaited without holding that mutex. Each direction retains its retry
  arguments and internal TLS buffers. OpenSSL may require read progress from a
  write and write progress from a read; executable coverage must validate the
  pending-operation interleavings before these guarantees are considered
  established.
- A successful send means the entire plaintext was accepted by the TLS engine
  and all ciphertext emitted for the operation was accepted by Network's
  complete-span send operation. It does not promise peer receipt or processing.
- A cancelled/timed-out receive retains accepted ciphertext/parser state where
  the backend permits retry. An interrupted send after entering OpenSSL is
  conservatively terminal for the channel because transmission may be partial.
  Raw transport EOF before authenticated `close_notify` is reported as
  truncation. A received `close_notify` is reported as `peer_closed` but leaves
  the channel available for explicit graceful shutdown; callers that abandon
  it may still choose abortive `close()`.
- `shutdown()` exchanges `close_notify`, then shuts down the TCP write
  direction and retires the transport. Cancellation, deadline, or transport
  failure during the exchange closes the channel; there is no retry after a
  partial shutdown exchange.
- Current CMake intentionally gates the experimental target to Linux with the
  existing Network provider and OpenSSL 3. No other platform is currently
  advertised or runtime-qualified.

## 2. Proposed invariant register

| ID         | Proposed invariant                                                                                                                                                                                             | Enforcement mechanism required                                                                                                                                                    | Concrete acceptance test                                                                                                                                   |
| ---------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| T-OWN      | A successful TLS construction atomically claims exclusive transport ownership. Ordinary TCP starters and mutators cannot use the claimed transport.                                                            | Private lease token and one shared `TcpConnectionState` mutex guarding claim, read/write starter admission, `close`, and `shutdown_write`.                                        | Race claim against read/write start repeatedly; exactly one side wins, and the losing path performs no native I/O.                                         |
| T-STALE    | A TCP operation created before claim but started after claim cannot bypass the lease.                                                                                                                          | The operation starter rechecks lease state under the same state mutex before reserving a direction.                                                                               | Create dormant receive/send operations, claim, then start them; each terminates with the documented ownership error and leaves TLS transport usable.       |
| T-AUTH     | Application plaintext is unavailable before role-appropriate authentication policy succeeds.                                                                                                                   | TLS session state gates read/write publication; backend verification result and reference-identity checks must both succeed where required.                                       | Trusted matching peer succeeds; untrusted chain and mismatched identity fail; no plaintext callback/result occurs before handshake success.                |
| T-SERIAL   | A backend session is never entered concurrently, even when one logical read and one logical write coexist.                                                                                                     | One per-channel protocol driver serializes backend calls and coordinates any read/write wants; never hold a mutex across a Network suspension.                                    | Instrument/mock backend to fail on overlapping entry; exercise concurrent read/write and WANT_READ/WANT_WRITE transitions.                                 |
| T-BUFFER   | Ciphertext, plaintext, and caller buffers remain valid until the relevant child and parent operations are terminal.                                                                                            | TLS-owned bounded staging buffers; parent operation state retains child Network operation/coroutine and borrowed spans until terminal arbitration.                                | Cancellation at every child-I/O phase under ASan; assert buffers remain live until child completion and no late access occurs.                             |
| T-COMP     | Each public TLS operation publishes exactly one `Completion<T>`; child completion is observed/drained before parent completion.                                                                                | TLS operation state with one terminal claim and explicit child-operation phase; reuse Network's terminal completion behavior.                                                     | Race native success, cancellation, deadline and transport failure; assert one stable terminal result.                                                      |
| T-CANCEL   | Cancellation is a cooperative request, not interruption. An operation that has committed protocol state either safely retains it or invalidates/closes the channel.                                            | Standard token/deadline passed to child operations; driver-owned state records partial records, plaintext and transmitted ciphertext; conservative close on unrecoverable output. | Cancel during partial handshake/read/write records; verify no reuse unless state-preservation proof succeeds.                                              |
| T-SEND     | TLS write success means all plaintext was consumed by the TLS engine and all generated ciphertext for that write was accepted by Network's socket send operation; it does not mean peer receipt or processing. | Track backend-consumed plaintext and drain all ciphertext associated with the operation before success.                                                                           | Loopback writes under backpressure; verify success only after complete ciphertext socket acceptance, and explicitly do not infer peer application receipt. |
| T-CLOSE    | Valid `close_notify` and transport EOF without `close_notify` are distinct outcomes.                                                                                                                           | TLS parser/session close state distinguishes authenticated TLS shutdown from raw TCP EOF.                                                                                         | Peer sends close_notify then FIN (clean); peer FIN without close_notify (truncation failure).                                                              |
| T-CAP      | TLS-owned buffering and backend memory are bounded; exhaustion is a defined error and cannot silently drop/reorder bytes.                                                                                      | Explicit per-channel limits and accounting before accepting plaintext/ciphertext; configured maximums validated at creation.                                                      | Fill each bound deterministically; expect resource-exhaustion failure or backpressure as specified, with byte order preserved.                             |
| T-PROGRESS | TLS never blocks a Network progress thread and never creates hidden progress resources. Cooperative mode advances only under Network polling.                                                                  | Nonblocking backend calls and Network-owned child operations; driver resumes only through Network operation completion/progression.                                               | Cooperative loopback makes no progress without poll and completes with poll; background test proves continuation workers can advance unrelated work.       |
| T-PORT     | A target is advertised only after backend, trust, entropy, runtime, and transport behavior qualify there.                                                                                                      | Explicit backend/platform capability configuration; unsupported creation fails explicitly.                                                                                        | Per-target installed consumer and TLS client/server conformance suite; unsupported target asserts exact unsupported-capability error.                      |

The error identity spellings and public API shape are not frozen by this
document. The proposed `vix.tls` error domain should provide distinct stable
identities for at least: invalid configuration, unsupported capability,
transport ownership conflict, authentication failure, peer identity mismatch,
protocol failure, unexpected EOF/truncation, resource exhaustion, and
backend failure. Cancellation remains `CompletionStatus::Cancelled`; deadline
expiry is an error identity and must not be confused with cancellation. Backend
diagnostics may supplement, but must not replace, stable identity.

## 3. Transport ownership and construction

### Proposed lease protocol

TLS construction receives an rvalue `TcpConnection` and an explicit security
configuration. Before changing transport ownership, it validates all
synchronous configuration and prepares the TLS channel state needed for the
claim. A private Network integration point then performs one atomic claim:

1. Lock the shared `TcpConnectionState` mutex.
2. Reject if the transport is closed, already claimed, or has an active read or
   write operation. Failure leaves the caller's TCP ownership unchanged.
3. Otherwise mark it TLS-leased and transfer a private lease token/state to
   TLS before releasing the mutex.
4. Move the public `TcpConnection` to its moved-from state only after the lease
   succeeds. After success the caller cannot perform TCP I/O or close through
   that handle; TLS owns eventual transport shutdown/close.

The exact representation is private. TLS must not receive a raw native socket
or bypass Network's operation/progression machinery. The lease should expose
only the internal operations needed to submit receive, send, write-half-close,
and final close through the owning provider.

All ordinary `TcpConnection` operation starters must check the lease flag under
the same mutex before setting direction-active state. This is required even
though the public connection is move-only: a previously created but unstarted
`Operation` stores a starter closure retaining the connection state and can be
started after the public handle is moved. Without the starter check, the lease
would not be exclusive.

### Race outcomes

- **Already-active read or write:** the active direction remains reserved
  through native I/O and terminal cleanup; claim rejects without mutation.
- **Operation created but not started:** claim may win; later starter sees the
  lease and terminates with the chosen stable ownership-conflict error without
  touching the descriptor or borrowed span.
- **Starter and claim concurrent:** both serialize on the state mutex. Starter
  first reserves the direction and claim rejects; claim first sets the lease
  and starter rejects. There is no intermediate state.
- **Native I/O in flight:** the direction slot remains active while the native
  call is outside the mutex; claim sees the reservation and rejects. The lease
  cannot close or replace the descriptor until that operation retires.
- **Cancellation callback registered:** Standard cancellation callbacks are
  synchronous and may run immediately on registration after prior
  cancellation. Network callbacks post cancellation facts to the provider;
  they do not directly complete native I/O. An active operation still blocks
  claim until its child operation is terminal and its borrowed buffer is
  released.
- **Failed claim:** no ownership transfer, TLS I/O, or descriptor mutation.
  The caller can continue using or close the original connection.
- **Provider shutdown:** the lease must retain Network's provider/resource
  accounting so shutdown remains rejected while the TLS channel or child
  operations are live, consistent with Network's existing resource leases.
- **Destruction after claim:** TLS owns the transport. It must safely cancel
  pending TLS child work, drain child operations before releasing their spans
  and coroutine state, then close through Network. It must not detach or close
  a descriptor behind an active child operation.

Moving/destroying the same C++ `TcpConnection` object concurrently with a
method call is a caller data race and is not made safe by the lease. The
lease protocol must, however, synchronize claim against independent operation
starter closures already created from that connection.

### Required Network change

The private I0-A prototype adds this lease in Network's non-installed engine
header. It is not a public Network API. `TcpConnectionState` has a mutex
guarding `read_active_`, `write_active_`, closed state, and direction
reservation; its starter closures reserve directions under that mutex
([Network.cpp](../network/src/Network.cpp:481)). The prototype adds a TLS-owned
bit and rechecks it from ordinary starters so dormant operations cannot bypass
the claim. Remaining work is executable race/lifecycle qualification, not a
public Network API.

## 4. TLS channel state and bidirectional progression

### Logical operations versus protocol steps

The public model should allow at most one logical read and one logical write
at a time per channel, matching Network's directional single-operation slots.
Handshake and channel shutdown are exclusive control phases. A logical
application operation may require several internal protocol steps: a write can
need inbound records (renegotiation/key-update/control traffic), and a read can
need outbound protocol records. The backend may indicate that a read or write
is currently required regardless of the initiating logical operation.

Network's separate TCP read and write slots only permit simultaneous transport
I/O. They do not make one TLS session safe for simultaneous backend calls.
TLS therefore needs one per-channel serialized protocol driver that owns all
mutable backend session state. It may keep one Network receive and one Network
send outstanding concurrently, but it must never call into the backend
concurrently.

### Driver requirements

The driver must:

- Invoke only nonblocking backend operations; no blocking socket adapter,
  hidden thread, or synchronous wait is permitted.
- Serialize engine entry and state changes. If background Network continuations
  can run on different workers, a per-channel pump-ownership/queued-work
  handshake must ensure one driver turn at a time and prevent lost wakeups.
- Never hold a channel mutex across `co_await`, callback invocation, or a
  Network operation that may reenter TLS. State is captured/committed under
  short locks; all child operations run outside locks.
- Translate backend “need input”/“need output” conditions into Network-owned
  receive/send children. A write waiting for input must allow its receive child
  to progress; a read waiting for output must allow its send child to progress.
- Retain protocol bytes and plaintext in bounded channel buffers, and retain
  the parent operation state until all required child operations have
  terminalized.
- In cooperative mode, execute driver turns only as a consequence of the
  caller-driven Network operation progress/resumption on the provider owner
  thread. It must not create a TLS worker or assume progress without `poll()`.

This is TLS-specific protocol orchestration, not a second reactor or generic
operation scheduler. The exact no-lost-wakeup handoff and backend adapter are
TLS-I0 feasibility questions.

### Buffer and backpressure contract

TLS must have explicit, finite limits for encrypted input, encrypted output,
decrypted-but-undelivered plaintext, and any backend-owned staging. The backend
must be configured so it cannot allocate unboundedly outside Vix's accounting,
or its allocation behavior must be bounded and documented before use. Defaults
and configurability require TLS-I0/I1 review; no values are selected here.

When output capacity is full, the driver waits for the Network send child to
consume output before asking the backend for more. When plaintext capacity is
full because the caller has not drained it, TLS stops further protocol input
that could expand plaintext. It must not drop bytes, reorder records, or
report successful application writes whose ciphertext remains unboundedly
queued. Capacity exhaustion before acceptance is a synchronous `Result`
failure; exhaustion after acceptance must be represented by a defined
terminal error or safe backpressure, never silent loss. The exact split is an
implementation/API decision to settle in TLS-I0.

**Send completion:** a successful TLS write means the backend consumed the
requested plaintext and the complete ciphertext generated for that logical
write was accepted by Network's socket send operation. It does not prove remote
receipt, TLS peer processing, or peer application consumption. If any committed
ciphertext send terminates unsuccessfully, the channel is unusable unless the
implementation proves a complete, protocol-safe recovery; v1 should close.

## 5. Handshake, authentication, and peer identity

Before handshake success, the channel is not authenticated and exposes no
application plaintext. A client handshake succeeds only after the configured
certificate-chain/trust policy and reference-identity policy succeed. A server
handshake establishes the server's local identity; it does not authenticate a
client unless an explicit mutual-TLS policy requires and validates a client
certificate.

The security configuration must distinguish:

- **Server identity:** the certificate/private-key identity presented by a
  server. The private-key ownership/loading mechanism must not leak key
  material through diagnostics or copyable public objects.
- **SNI:** the server name sent by a TLS client to select a virtual server.
  SNI is a protocol routing hint, not proof of peer identity.
- **Reference identity:** the DNS name or IP identity the client intended to
  reach and against which the authenticated certificate is checked. It must
  not be inferred from SNI or omitted merely because Network connects to a
  numeric `Endpoint`.
- **Trust roots:** the configured trust anchors and policy. The source and
  lifecycle of system roots, bundled roots, and caller-provided roots are not
  selected; trust must never silently fall back to “accept any certificate.”
- **Protocol policy:** minimum/maximum TLS versions and any protocol-neutral
  ALPN offer/selection. TLS version defaults require backend security review;
  ALPN does not make TLS HTTP-specific.

Authentication failures are terminal handshake failures. No caller may opt
into unauthenticated operation through an accidental empty trust configuration.
If an explicit insecure testing mode is ever considered, it requires separate
human approval, conspicuous naming, and exclusion from secure-default examples;
it is not part of this contract proposal.

## 6. Cancellation, deadline, and terminal arbitration

TLS operations use Standard `CancellationToken`, `Deadline`, and
`Completion<T>`. A deadline is absolute and monotonic. Child Network
operations receive the same token and deadline, subject to an earlier parent
deadline. Network already arbitrates a native result observed before
cancellation/deadline; TLS must consume that child completion and then perform
its own one-winner parent arbitration. The application outcome is never
rewritten after terminal publication.

| Event/state                                                          | Required retained or invalidated state                                                                                                                                                    | Parent outcome and reuse                                                                                                                                                                                               |
| -------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Cancellation before lease claim                                      | No TLS state owns transport; original TCP connection remains caller-owned.                                                                                                                | No TLS operation exists. A failed synchronous construction/claim returns `Result` failure and leaves TCP usable.                                                                                                       |
| Cancellation after claim, before any handshake backend call or I/O   | Retain only an unadvanced session if the backend proves it can be cleanly reconstructed; otherwise discard session. Transport is TLS-owned.                                               | Handshake completes Cancelled. Reuse is allowed only if no bytes entered/left and backend state is proven untouched/reconstructible; otherwise close.                                                                  |
| Cancellation/deadline during handshake                               | Drain current Network child; retain no externally visible plaintext. Handshake transcript/session is not reusable unless backend proves a restart-safe state, which v1 should not assume. | One Cancelled or deadline failure wins; close transport after child terminal.                                                                                                                                          |
| Partial ciphertext record received during read                       | Retain ciphertext/parser state until read continues; do not discard bytes if channel is to remain open.                                                                                   | If cancellation occurs, operation may complete Cancelled only after child I/O is terminal. Channel is reusable only if parser state is retained exactly; otherwise close.                                              |
| Plaintext decrypted but not delivered when read is cancelled         | Retain plaintext in bounded channel buffer; do not overwrite/reuse caller span or expose it twice.                                                                                        | Cancelled parent may leave channel reusable only if buffered plaintext and TLS sequence state are preserved. Otherwise close.                                                                                          |
| Ciphertext generated but not yet submitted during write cancellation | Backend sequence state may already have advanced; retain complete ciphertext and commit state or invalidate channel.                                                                      | V1 policy: if the logical write had entered backend and generated committed ciphertext, cancellation/deadline closes the channel unless the driver can finish the complete record under an explicitly approved policy. |
| Cancellation during Network send of TLS ciphertext                   | Network may have transmitted a prefix but reports no prefix count on failure. Keep send buffer alive through terminal child completion.                                                   | V1 closes after child terminal; never retry the same record on a possibly advanced TCP stream.                                                                                                                         |
| Native send success races cancellation                               | Child Network success is honored if it wins Network arbitration. TLS advances its ciphertext offset once and processes resulting protocol state.                                          | Parent operation then wins exactly once; if the complete logical write has drained, success may be published even if cancellation was requested later.                                                                 |
| Deadline expires during internal TLS protocol progression            | Keep driver, child operation, buffers, and parser/engine state alive until child terminal; propagate deadline to each child and check between backend steps.                              | Pending work does not begin after deadline; running operations request cooperative cancellation. No hard latency bound. Actual completed success remains success; unsafe partial output closes.                        |
| Explicit cancellation races deadline or transport error              | Preserve child Network's outcome and one TLS parent terminal winner. Do not issue duplicate child cancellation or publish twice.                                                          | One `Completion` only. A terminal child success may be processed before later cancel; failure/timeout invalidates the channel where protocol state is uncertain.                                                       |
| Peer sends close_notify                                              | Preserve any preceding plaintext, mark peer's TLS write direction closed, and distinguish clean TLS EOF from TCP EOF.                                                                     | Reads return clean end-of-stream after buffered plaintext drains; channel may still support local writes if protocol/backend permits.                                                                                  |
| Peer TCP EOF without close_notify                                    | Preserve no claim of authenticated end-of-stream; report truncation/unexpected EOF.                                                                                                       | Channel is invalid and must close; do not return clean TLS EOF.                                                                                                                                                        |
| Transport fails during graceful shutdown                             | Keep close-notify ciphertext buffer and child operation alive until terminal.                                                                                                             | Return transport failure; channel closes and cannot be reused.                                                                                                                                                         |

### Write-cancellation policy

Two policies were considered:

- **A — close after interrupted in-flight ciphertext transmission.** This is
  conservative and matches Network's contract: failed send may have transmitted
  a prefix, but no byte count is returned. It gives cancellation a clear
  terminal boundary and avoids replaying a record on an uncertain stream.
- **B — finish a committed TLS record internally before completion/close.**
  This can preserve framing, but may ignore cancellation/deadline while
  backpressured, can block shutdown indefinitely, and requires precise knowledge
  of backend record commitment and transport partial-send state.

Proposed v1 policy: A. Cancellation is not forced interruption; after the
current Network child reaches terminal state, the TLS parent publishes once
and closes an invalidated channel. The driver must not detach a sender or
continue hidden output after the parent has completed.

## 7. Shutdown, buffers, and completion ordering

Every started TLS operation owns its parent state until terminal completion.
That state owns/retains all Network child operations, backend input/output
storage, caller-borrowed plaintext spans, and coroutine frames needed by those
children. The TLS parent must not publish terminal completion until every child
operation it depends on has completed and no child can access those buffers.
Destroying a started incomplete Network awaiter remains forbidden under the
existing Network contract; TLS cannot weaken or bypass that rule.

Cancellation requests child cancellation but do not authorize destruction of
the child awaiter or buffer. For cooperative providers, child progression and
coroutine resumption remain on the provider owner thread through `poll()`.
Background mode may resume continuations on Network-owned workers; the TLS
driver must be safe under that model without blocking those workers.

Graceful TLS shutdown must:

1. Stop accepting new application writes and serialize against the current
   logical write.
2. Ask the backend to produce TLS `close_notify` and drain the complete
   notification through Network send.
3. Only after the close-notify bytes are accepted by the transport, request
   Network write-half shutdown. This indicates local TLS write closure; it does
   not prove peer receipt.
4. Continue to distinguish a peer `close_notify` from raw TCP EOF. Whether the
   public graceful-shutdown operation also waits for peer close-notify is an
   unresolved API decision; an unbounded wait must not be hidden inside
   destruction.

Abnormal destruction or unrecoverable protocol/transport failure closes the
leased transport through Network once child operations are safely terminal.
It must not silently report graceful TLS closure. Destruction must not block a
Network continuation worker waiting on work that requires that same worker;
the exact public close/destructor contract is a TLS-I0/I1 feasibility gate.

## 8. Capacity and resource exhaustion

TLS buffering is per channel and bounded. Candidate separately accounted
limits are: encrypted ingress, encrypted egress, decrypted plaintext awaiting
the caller, backend internal staging, and handshake/control overhead. The
backend must provide documented bounds or be wrapped with limits that prevent
unbounded accumulation. The API must either configure finite limits or use
reviewed finite defaults; “unbounded” is not an acceptable implicit default.

Backpressure must stop additional reads/writes when a downstream bounded buffer
is full. If a limit is reached while an already-accepted operation cannot
progress, the operation returns a stable resource-exhaustion error and the
channel closes unless all buffered protocol state remains valid and reusable.
The exact recoverability rule must be backend-specific evidence, not assumed.
Admission must not consume caller buffers or accept an operation if capacity
cannot be reserved synchronously. These details and numeric defaults remain
for TLS-I0/I1 approval.

## 9. Platform and backend policy

Network's CMake selects platform source families for Linux, Android, macOS,
Windows, and wasm32-wasip2, but that source selection does not establish TLS
backend availability or runtime conformance
([network CMake](../network/CMakeLists.txt:11)). Network's platform matrix
explicitly distinguishes builds from runtime evidence
([PLATFORM_VALIDATION.md](../network/PLATFORM_VALIDATION.md:1)). TLS must do
the same.

No platform is TLS-qualified by this proposal. A platform row may be marked
supported only after the selected backend is built with the target toolchain,
trust and entropy sources are validated, and client/server runtime tests cover
authentication, cancellation, deadlines, partial I/O, close-notify, and
truncation. WASI Preview 2 is explicitly unsupported until the cooperative
backend and host capability path are proven. Unsupported platforms must return
an explicit capability failure rather than silently falling back to plaintext.

## 10. Decisions to approve before TLS-I0

1. **Lease error identity:** choose a stable public/internal identity for an
   operation created before claim but started afterward; preferred distinction
   is `transport_owned`, separate from ordinary `operation_in_progress`.
2. **Read interruption recovery:** require all qualified backends to preserve
   partial records and decrypted-but-undelivered plaintext, or conservatively
   close on any interrupted read that cannot prove preservation. Recommended:
   preservation is mandatory for reusable channels; otherwise close.
3. **Pre-handshake cancellation:** allow retry only if no backend state or
   transport byte was advanced and the session can be reconstructed; otherwise
   close. Recommended default is close unless feasibility evidence makes the
   untouched state observable.
4. **Shutdown split:** decide whether v1 exposes separate local write
   close-notify/half-close and peer close-notify wait, or one operation with
   explicit bounded/deadline semantics. Destruction must never wait for peer.
5. **Buffer limits:** select finite defaults, per-channel configurability,
   exhaustion identities, and whether exhaustion is recoverable.
6. **Authentication configuration:** select system/bundled/caller trust-root
   source, DNS/IP reference-identity representation, server key/certificate
   ownership, client-auth policy, and secure TLS version defaults.
7. **Backend and platforms:** compare maintained backend candidates for
   nonblocking in-memory I/O, verification, diagnostics, exception-disabled
   builds, licensing, installed SDK packaging, and every target. Do not select
   by familiarity alone.
8. **Operation abandonment/destruction:** define what happens if a TLS
   coroutine owner is destroyed while its operation is incomplete; this must
   not violate Network awaiter lifetime or block a required progress worker.

## 11. Candidate conformance tests

Before any platform is called TLS-supported, its test suite must prove:

- Lease races and dormant-operation rejection as listed under T-OWN/T-STALE.
- Authenticated client success and wrong-identity/untrusted-peer failure,
  plus server identity and optional mutual-auth behavior.
- No plaintext before handshake; correct encrypted round trip and integrity
  failure behavior.
- Concurrent logical read/write with backend wants for both directions and
  serialized backend entry.
- Bounded memory/backpressure behavior and deterministic exhaustion.
- Cancellation/deadline at every row of the state matrix, with exactly-once
  completion, child lifetime, buffer lifetime, and correct channel reuse/close.
- Clean close-notify, peer close-notify, and truncation on raw EOF.
- Background and cooperative Network modes; cooperative mode must not progress
  without polling and must not create hidden threads.
- Provider shutdown remains blocked/rejected while TLS leases or child
  operations exist, then succeeds after safe retirement.
- Installed SDK consumer builds against only public targets and headers; no
  backend-private headers or build-tree paths leak into the package.
