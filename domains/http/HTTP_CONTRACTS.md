# Vix HTTP Domain — H0 Contracts and Simplicity Audit

Current status (2026-10-19): the experimental Linux-only `vix::http_experimental`
component installs `vix/http/Http.hpp`; the separately selected
`vix::http_tls_experimental` integration installs `vix/http/Https.hpp` and
requires the experimental TLS component/OpenSSL 3. HTTPS uses explicit TLS
configuration and never falls back to plaintext. Both client and one-request
server APIs use bounded owned messages and `network::run`; HTTPS handshakes
before HTTP data and completes TLS shutdown before successful server response
completion. These are provisional package capabilities, not stable API or
cross-platform qualification. DNS, streaming, persistence, retries, redirects,
upgrades, and CONNECT tunneling remain unsupported. The prior H0-P4 security and
private-proof status below is historical evidence. See
[HTTP_EXPERIMENTAL.md](HTTP_EXPERIMENTAL.md) and
[HTTP_HTTPS_EXPERIMENTAL.md](HTTP_HTTPS_EXPERIMENTAL.md) for current boundaries.

Historical status: private H0-P1 parser/framer, H0-P2B TCP, and H0-P3B
Linux-only authenticated HTTPS proofs. H0-P4B semantic repairs are
implemented; the security review below remains a private prototype assessment,
not complete security/package qualification. At that checkpoint there was no
public API or stable installed/exported HTTP target. The HTTPS proof is
opt-in, test-only, and limited to explicitly framed responses; it does not
qualify cross-platform TLS-over-HTTP support.

## H0-P1 implementation evidence (private, not a public contract)

### Private exchange resource bounds (H0-P4-S1)

Private TCP and HTTPS exchanges accept at most 128 informational responses
before the final response. The 128th is permitted; completing a 129th
terminates the exchange with the distinct `vix.http:5`
`informational_response_limit_exceeded` identity and closes/invalidates that
connection. The count belongs to the exchange, not the resettable parser, and
is retained across receive calls. `101 Switching Protocols` remains a separate
unsupported transition error.

Each private exchange also receives one absolute 30-second default deadline,
created at exchange admission. A caller-provided deadline is combined with it
using the earlier boundary, so a stricter caller deadline is preserved and a
later one cannot extend the private cap. The same boundary covers TLS
handshake, request transmission, response reception, and TLS shutdown. It is
best-effort monotonic timing, not a hard real-time bound. Expiry propagates
through existing Network/TLS operations as `vix.network:21` (`timed_out`);
terminal cleanup closes the ambiguous exchange/channel rather than beginning
an unbounded graceful cleanup operation. Child operations still retire under
their existing contracts before parent completion and transport release.

These limits bound one exchange's informational-response work and active
lifetime; they do not constitute a process-wide connection quota, a bound on
all concurrent exchanges, or a hard bound on cleanup latency if a lower-layer
provider violates its own retirement contract.

The private engine in `src/Http1.hpp` and `src/Http1.cpp` parses request and
response messages incrementally, exposes body bytes into caller storage, and
retains unconsumed/surplus bytes. Its test-only build is disabled unless
`VIX_BUILD_HTTP_P1_TESTS=ON`. It has no Network, TLS, OpenSSL, install, or
export dependency. A focused build with `VIX_ENABLE_WARNINGS=ON` and the
protocol test passed locally with GNU C++ 13.3.0; that result is not
cross-platform or RFC conformance qualification.

The prototype currently supports HTTP/1.0 and HTTP/1.1 syntax, a single
`chunked` transfer coding, fixed-length bodies, close-delimited responses,
no-body responses, and successful CONNECT tunnel boundary recognition. It
does not provide a live tunnel API. Its private default bounds are 8 KiB
start/status lines, 32 KiB headers and 100 header fields, 4 KiB chunk lines,
64 KiB cumulative chunk metadata, 8 KiB trailers and 32 trailer fields, and
64 KiB staged input. These are proof-of-concept limits, not proposed stable
configuration defaults.

### Framing decisions demonstrated by the prototype

| Input/context | Private decision | Basis |
| --- | --- | --- |
| Response to HEAD; 1xx, 204, or 304 response | No message body, regardless of `Content-Length`; forbidden `Transfer-Encoding` on 1xx/204 is rejected | RFC 9112 §6.3, §6.1 |
| Successful 2xx response to CONNECT | Transition to opaque tunnel boundary after headers; ignore CL/TE as required | RFC 9112 §6.3 |
| One or repeated equal decimal Content-Length values, including equal comma-list values | Accept one length | RFC 9112 §6.3 recipient recovery rule |
| Invalid, negative, overflowing, or conflicting Content-Length | Reject; no resynchronization | RFC 9112 §6.3 plus conservative connection handling |
| Transfer-Encoding together with Content-Length | Reject as ambiguous rather than applying TE precedence; successful CONNECT is the protocol-defined exception above | RFC 9112 §6.3 permits/encourages rejection; anti-smuggling policy |
| HTTP/1.0 with Transfer-Encoding | Reject (except successful CONNECT response handling occurs first) | RFC 9112 §6.1 |
| Transfer-Encoding without CL | Accept only a single `chunked` coding; reject unsupported or repeated codings | RFC 9112 §6.1, §7.1; conservative subset |
| Request without CL/TE | Zero-length body | RFC 9112 §6.3 |
| Response without CL/TE and not a no-body/tunnel case | Close-delimited body; complete only on EOF after staged bytes are delivered | RFC 9112 §6.3 |
| Fixed or chunked body reaches EOF early | `truncated_message` error | RFC 9112 §8 |
| HTTP/1.1 request Host | Exactly one syntactically valid Host required; duplicate/missing/invalid rejected. Empty field value is permitted when the target URI has no authority component. | RFC 9112 §3.2 |
| Whitespace before colon, invalid token/value, bare LF, request obs-fold | Reject | RFC 9112 §5; stricter recipient policy where RFC permits leniency |
| Response obs-fold | Replace fold with one SP before field interpretation | RFC 9112 §5.2 recipient requirement |
| One initial empty request line | Ignore; further leading empty line is rejected | RFC 9112 §2.2 server SHOULD behavior, with bounded conservative parsing |

All message syntax is parsed as octets. Once an error occurs, parser state is
sticky and bytes are not reconsidered as another message. `reset_message()`
is available only after a complete non-tunnel message; callers must still
decide connection disposition and must close after malformed/ambiguous input.
The serializer validates request-target forms, Host syntax, response status
lines (including the mandatory separating SP for an empty reason phrase), and
field syntax; it owns framing-field generation and refuses contradictory
framing and CR/LF injection. Serialization preflights bounded start-line,
header, and trailer sizes before growing its output buffer; rejected output
leaves the caller's prior output unchanged. It rejects generated 205 content
(while allowing an explicit zero Content-Length) and non-final chunks cannot
carry trailers. This private policy needs human review before it can become a
network-facing contract.

The expanded local corpus now includes fixed-seed randomized fragmentation,
4,000-plus one-byte fragments, EOF at every prefix boundary of fixed and
chunked messages, sticky failures, malformed request targets, serializer
round-trips, and Release-mode checks that retain active `CHECK` assertions.
These deterministic tests are not fuzz qualification.

## 1. Purpose and boundary

HTTP should exist as Vix's reusable, standards-conforming capability for
constructing, parsing, sending, receiving, and streaming HTTP messages over a
byte-stream transport. Its value is that applications do not have to
reimplement message syntax, framing, parser state, connection reuse rules, or
the security-sensitive handling of ambiguous input.

HTTP is not an application framework. It does not choose routes, handlers,
middleware, authentication or authorization policy, JSON representation,
templates, dependency injection, or application lifecycle. It does not own a
reactor, scheduler, worker pool, or independent socket implementation.

The primary protocol baseline is HTTP semantics in [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html)
and HTTP/1.1 message syntax and connection management in
[RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html). The implementation
must follow the normative specifications, applicable verified errata, and
updates such as [RFC 9931](https://www.rfc-editor.org/rfc/rfc9931.html), not a
convenient subset that silently changes message meaning. The verified RFC
9112 errata are editorial (including the `obs-text` cross-reference and
erratum 8284's placement of the absolute-form/Host priority note); they do not
change this parser's octet or framing rules. In future origin-form handling,
absolute-form request-target authority takes precedence over `Host` for
routing, as RFC 9112 §3.2.2 specifies. The private P1 parser validates syntax
but intentionally does not perform routing or authority comparison.

RFC 9931 adds security requirements around optimistic HTTP/1.x protocol
transitions. P1 only recognizes the 101 and successful CONNECT boundary; it
does not implement Upgrade or tunnel protocols. Any later transition support
must not send protocol-specific bytes before the transition is confirmed.

## 2. Existing Vix simplicity and contracts

### Standard — established facts

- `vix::Result<T>` represents synchronous success or an expected `Error`;
  it is not an asynchronous state or cancellation type
  (`standard/include/vix/Result.hpp`).
- `vix::Completion<T>` represents one terminal success, failure, or
  cancellation. The provider/operation state machine arbitrates races before
  publishing it (`standard/include/vix/Completion.hpp`).
- `vix::Error` uses a stable `ErrorIdentity` for program logic; message and
  context are diagnostic (`standard/include/vix/Error.hpp`,
  `standard/include/vix/ErrorIdentity.hpp`).
- `CancellationToken` callbacks execute synchronously on the thread that
  delivers cancellation; registration can call back immediately when already
  cancelled (`standard/include/vix/Cancellation.hpp`).
- `Deadline` is an absolute monotonic boundary, not itself a timeout outcome
  (`standard/include/vix/Deadline.hpp`).

These are composable and clear contracts. HTTP should return synchronous
construction/admission failures as `Result`, terminal operations as
`Completion`, and use Standard cancellation/deadlines through the existing
Network operation boundary. It should not wrap a failure in a second result
channel or define an HTTP-specific cancellation type.

### Concurrency — simplicity audit

The public Concurrency model is resource-owned work, not a public pool or
scheduler: `ExecutionResource::create`, bounded `submit`, `WorkHandle<T>`,
`WorkContext`, `WorkOptions`, and one-shot `NotificationRegistration`
(`domains/concurrency/include/vix/concurrency/ExecutionResource.hpp`). It
genuinely hides worker creation, queueing, deadline observation, completion
publication, and callback dispatch. The implementation's worker/deadline
machinery stays private (`domains/concurrency/src/ExecutionResource.cpp`).

The API is not “no concepts”: callers still choose capacity, retain
`WorkHandle`s when they need control/observation, understand that cancellation
is cooperative, keep borrowed captures alive, avoid blocking a reactor on
unfinished work, and explicitly shut down a resource that may wait
indefinitely. `NotificationRegistration` has a detailed quiescence contract.
Those are real ownership and progress responsibilities, not merely leaked
thread-pool details; however, this surface is more advanced than HTTP should
expose to ordinary request users. HTTP must not require an `ExecutionResource`
or make users schedule protocol steps themselves.

### Network — simplicity audit

The public surface consists of `Provider`, `Endpoint`, `TcpConnection`,
`TcpListener`, `Operation<T>`, and `OperationOptions` in
`domains/network/include/vix/network/`. A caller composes connect/accept,
receive/send and `co_await`; `Provider::create_cooperative()` makes the
progress rule explicit (`domains/network/COOPERATIVE_PROGRESS.md`). Network
owns sockets, readiness, operation progression, cancellation/deadline
arbitration, and provider accounting. `TcpConnection` is move-only and its
spans are borrowed until operation terminal completion. An incomplete started
awaiter may not be destroyed; cooperative progress requires owner-thread
`poll()` calls. These are justified low-level contracts because they bound
resource and buffer lifetimes and distinguish two actual progress modes.

Network is simpler than exposing readiness events, but a user still must
understand awaiter lifetime, borrowed spans, provider lifetime, cooperative
polling, and whether an operation can partially send before failure. HTTP's
normal client/server path should hide those protocol-specific mechanics while
preserving the underlying contracts.

### TLS — simplicity audit

The build-tree public TLS surface is small: owned client/server configuration,
`TlsChannel::create_client/create_server`, then `handshake`, `receive`, `send`,
`shutdown`, or abortive `close` (`domains/tls/include/vix/tls/`). The OpenSSL
driver and Network lease are private. This successfully hides SSL/BIO calls,
record parsing, backend retry rules, and transport claiming. Users still must
choose trust roots and expected peer identity, understand that SNI and
identity verification differ, preserve borrowed I/O buffers until completion,
handle cancellation/deadlines, distinguish close_notify from raw EOF, and
choose graceful versus abortive close. These are security and ownership
decisions that should remain visible.

TLS documentation explicitly marks its implementation experimental,
Linux-only, opt-in, and not installed/exported yet
(`domains/tls/TLS_CONTRACTS.md`, `domains/tls/TLS_IMPLEMENTATION_PLAN.md`).
Therefore HTTP may not imply that HTTPS is available in the stable SDK. No
plaintext fallback may occur when a caller requests HTTPS.

### Simplicity conclusion

The essential concepts HTTP users need are message/request/response, a body
source or sink for streaming, and a client exchange or server accept/exchange
operation. Parser states, framing counters, chunk syntax, BIOs, transport
leases, worker queues, and readiness events are implementation mechanics.
Security identity, cancellation/deadline, body ownership, and explicit
streaming backpressure cannot be hidden without obscuring meaningful caller
obligations.

## 3. HTTP responsibility split

### HTTP Domain owns

- HTTP/1.1 request/status line and field syntax, validation and serialization.
- Message semantics needed to process HTTP/1.1 correctly, including methods,
  statuses, fields, target form, persistence, and protocol switching rules
  that apply even when an extension is unsupported.
- Message framing and body boundaries: Content-Length, chunked transfer
  coding, no-body cases, and close-delimited responses where permitted.
- Incremental parsing and writing, with explicit memory limits and
  backpressure.
- Connection state: whether the next request/response may use a connection,
  whether it must be closed, and which bytes belong to which message.
- Rejection of malformed, conflicting, ambiguous, or unsupported framing
  before bytes can be reinterpreted differently by another hop.
- Machine-readable HTTP errors for malformed syntax, invalid framing,
  truncated messages, limits, unsupported protocol feature, and transport
  failure translation where required. Underlying Network/TLS failures should
  retain identity as a cause when the boundary translates them.

### Client policy owns

- Which origin/endpoint to contact, request method/fields/body, and what
  response statuses mean to the calling application.
- Redirect policy, retry policy, cookies, credentials, caching, proxy
  configuration, and connection-pool policy. These are not implicit in a
  single HTTP exchange and should not be silently performed by a minimal
  client API.
- Resource policy values such as body/header limits, connection limits, and
  application-level deadline choice, subject to safe protocol minimums.

### Server/application policy owns

- Which endpoint to listen on (Network), accepted-connection admission and
  application service behavior.
- Routing, handler dispatch, authorization, authentication, content decoding
  such as JSON, business logic, and response selection.
- Graceful server drain policy and overload response policy at the application
  boundary, while HTTP provides safe per-connection protocol behavior.

## 4. Proposed minimal developer model (not implemented)

The ordinary exchange should be a single HTTP operation over an existing
Network provider, with an owned request description and a response whose body
can be read incrementally. Construction/configuration errors use `Result`;
I/O terminal outcomes use `Completion`. The exact surface remains open until
the operation-construction seam is proven.

Illustrative client use:

```cpp
// Proposed API only; no such types or functions are implemented.
auto client_result = vix::http::Client::create(provider);
if (!client_result)
  return client_result.error();
auto client = client_result.take();

vix::http::Request request;
request.method = "GET";
request.target = "/status";
request.headers.add("accept", "text/plain");

auto exchange = client.send(endpoint, std::move(request), options);
vix::Completion<vix::http::Response> response = co_await std::move(exchange);
```

For a POST, request content is a body source rather than a mandatory
whole-body `std::string`:

```cpp
// Proposed API only. A bounded body source would yield bytes on demand.
request.method = "POST";
request.target = "/items";
request.body = vix::http::BodySource::from_bytes(payload);
auto completion = co_await client.send(endpoint, std::move(request), options);
```

The proposed server capability should accept a Network listener/connection,
parse one request at a time per HTTP/1.1 connection, then expose a request body
reader and a response writer. It should not introduce routing:

```cpp
// Proposed API only; the application owns dispatch and policy.
auto request = co_await server.accept_request(listener, options);
if (request) {
  auto body_chunk = co_await request.value().body().read(buffer, options);
  auto response = vix::http::Response::text(200, "ok");
  co_await request.value().respond(std::move(response), options);
}
```

Large payloads use pull-based bounded body source/sink operations. The
producer/consumer sees backpressure through operation completion; the parser
does not buffer an unbounded message. Exact callback-versus-awaitable body
surface is unresolved. Cancellation and optional absolute Deadline are passed
through one operation-options value backed by Standard and Network; cancelling
does not promise rollback of bytes already transmitted. A partially sent
request or response makes that HTTP connection non-reusable unless the engine
can prove message alignment.

## 5. Internal architecture and transport integration

### Shared protocol engine

Use one HTTP/1.1 engine for syntax, framing, connection-state transitions,
limits, and error decisions. Client and server are policy-facing facades over
that engine, not separate parser implementations. Keep the engine incremental
and transport-agnostic internally so byte framing can be tested without
sockets. This is not a reason to publish a general parser framework.

### Transport choice

The engine needs an ordered byte-stream interface internally: asynchronous
read some, write all/partial with exact progress, cancellation/deadline,
terminal child retirement, and close/half-close semantics. Plain HTTP maps to
Network `TcpConnection`; HTTPS can map to `TlsChannel` only when that TLS target
is present and qualified. An HTTP-internal adapter/type-erasure remains the
smallest candidate for a future public capability; a public general transport
concept is not justified by these private proofs, especially while TLS is
experimental.

There is a concrete async-composition seam: public `network::Operation<T>` is
awaitable, while its constructor and `detail::OperationFactory<T>` remain
private to Network's uninstalled `Engine.hpp`. The private HTTP proofs use this
factory so parent resumption and provider accounting remain Network-owned.
HTTP-over-TCP uses the distinct HTTP-tagged private claim; the H0-P3B HTTPS
proof instead consumes a moved `TlsChannel` and never makes a second TCP
claim. TLS retains exclusive ownership of its TCP lease. Neither protocol
claim can grant authority to the other.

These proofs do not authorize a public transport abstraction. Any later public
client/server capability still needs a reviewed private adapter boundary:

1. A private Network domain-composition factory can create parent
   `Operation<T>` whose resumption, cancellation, deadline, and cooperative
   progress remain governed by the same provider; or
2. HTTP owns a minimal compatible private awaitable/state adapter without
   duplicating reactor or provider progression.

Do not solve this by making `OperationFactory`, raw sockets, or a generic
transport base class public. A shared byte-stream *contract* is useful
internally to make HTTP protocol code independent of plain/TLS transport; a
shared public byte-stream *API* is not yet demonstrated. The private TCP and
TLS loopback proofs exercise the Network operation-creation and lifetime seam
on Linux, but do not qualify it as a durable public contract.

### TLS packaging rule

Plain HTTP build and installed SDK use must have no OpenSSL dependency.
Experimental/unqualified TLS must not leak into stable HTTP exports. When TLS
is unavailable, HTTPS selection returns an explicit unsupported-capability or
configuration error; it must never retry as plaintext HTTP. Separate targets
or optional link integration must ensure a plaintext consumer does not resolve
TLS/OpenSSL dependencies.

## H0-P2B implementation evidence (private feasibility test only)

`src/PrivateExchange.hpp/.cpp` and
`tests/private_exchange_test.cpp` prove one HTTP-owned parent operation over
the private HTTP-tagged Network lease. The parent owns request storage, parser
state, bounded receive/body storage, and the lease; it awaits Network child
operations before publishing its single terminal `Completion`. A received
response is parsed by the same P1 engine used by the parser corpus. The test
uses a real loopback TCP listener/client exchange in both background and
cooperative provider modes, including 11-byte receive buffers, truncation,
cancellation, deadline expiration, provider accounting, and rejection of a
cooperative operation started from a foreign thread. A private Network test
hook caps each native send span at three bytes in the successful exchange;
the test verifies multiple native sends and receives, exercising partial
progress in both directions. The hook is unbounded by default and is compiled
only for test builds. This is not a public client/server API or a
reusable-connection implementation.

After a completed send, the private parent posts its next receive progression
to the existing provider command queue. This avoids starting TCP descriptor
registration from a continuation while a stale readiness callback for the
previous direction may still be unwinding. No new thread or scheduler is
created. The handoff awaiter avoids touching its coroutine frame after the
continuation handle is published, because the provider may resume it before
`post()` returns.

The proof exposed and corrected a Network terminal-publication race: a child
could mark itself completed-before-suspend before storing its `Completion`,
allowing `await_resume()` to consume an empty result. Network now distinguishes
completion-in-progress from fully published pre-suspend completion; if
suspension commits during terminal cleanup, the child schedules exactly one
resume after completion storage and provider-operation retirement. A
deterministic private regression covers both race winners.

GNU C++ 13.3 strict Debug focused Network/HTTP/TLS tests passed 14/14; focused
ASan/UBSan Network/HTTP/TLS tests passed 5/5; the private exchange passed ten
consecutive strict Debug repetitions and five consecutive ASan/UBSan
repetitions. The successful exchange forces three-byte native send spans and
11-byte receive buffers and verifies repeated native progress in both
directions. These earlier results are superseded by the current H0-P3B
validation summary in the implementation plan. They are local Linux results
only. Platform qualification, a public operation/API design, streaming
request/response bodies, persistent connections, and installed-SDK integration
remain unimplemented. The private HTTPS proof rejects close-delimited
responses; it does not treat TLS close_notify as an HTTP framing substitute.

## H0-P4A source audit — current protocol boundary

This is a source/test audit of the private prototype, not RFC conformance or
cross-platform qualification. The focused parser corpus is
`tests/http1_protocol_test.cpp`; the TCP and HTTPS composition tests are
`tests/private_exchange_test.cpp` and `tests/private_tls_exchange_test.cpp`.
The matrix below records the P4A baseline findings. The H0-P4B dispositions
that follow supersede its marked gaps; it is retained as the audit trail.

| Area | Current behavior | Evidence and disposition |
| --- | --- | --- |
| Start lines and versions | Request/status lines accept HTTP/1.0 or 1.1 only, require exact SP structure and CRLF, validate methods/targets/reason bytes, and accept status codes 100–599. | Parser source and split/malformed-line corpus. Bare LF is rejected although RFC 9112 permits recipients to recognize it; this is a conservative rejection. HTTP/0.9, HTTP/2, and other versions are unsupported. |
| Header fields | Names must be tokens; whitespace before `:` and control/injection bytes are rejected; values are OWS-trimmed; field bytes/count are bounded. Request obs-fold is rejected. Response obs-fold is replaced by SP, as required for a user agent. | `parse_header()`, `take_line()`, malformed-field corpus. Header values retain obs-text; no Unicode decoding occurs. |
| Host | HTTP/1.1 requests require exactly one syntactically accepted Host; duplicate Host is rejected. Empty Host is accepted. HTTP/1.0 may omit Host. | Tests cover missing/duplicate/invalid HTTP/1.1 Host and valid empty Host. **Gap:** an invalid Host value on HTTP/1.0 is not passed through `valid_host()` and is accepted; e.g. `GET / HTTP/1.0\r\nHost: bad host\r\n\r\n`. RFC 9112 requires a server to reject an invalid Host value regardless of request version. Add a regression and validate every present Host before any public/server use. |
| Content-Length | Decimal values use checked `uint64_t` parsing. Repeated fields and comma lists are accepted only when every value is valid and numerically equal; conflicting, negative, empty, and overflowing values fail terminally. | Framing ambiguity corpus. This follows RFC 9112's permitted equal-list recovery; all other invalid values are rejected and the caller must close the connection. |
| Transfer-Encoding | Only one `chunked` coding is implemented. Unsupported codings/chains, repeated chunked, HTTP/1.0 TE, malformed lists, and TE+CL are rejected. | Framing corpus. Rejecting TE+CL is the conservative RFC 9112 recommendation. Other transfer codings are unsupported, not decoded. Successful CONNECT is handled first and ignores CL/TE as RFC 9112 requires. |
| Chunked bodies/trailers | Chunk sizes/extensions, data CRLF, zero chunk, and trailer termination are incremental and bounded; chunk-size overflow and malformed delimiters fail. Selected framing/routing/auth fields are denied in trailers; trailers remain separate and are never merged into headers. | Chunk/trailer tests cover splits, truncation, bad terminators, overflow limits and Content-Length trailer rejection. The static deny list is not a field-definition registry; callers must not merge arbitrary trailers. |
| Request framing | CL and chunked determine body framing independent of method; absent both means zero-length request body. | Request body tests. The parser does not enforce method semantic policies (for example, it does not reject content on CONNECT/TRACE); those belong to a higher HTTP role/policy layer. |
| Response body precedence | Response parsing requires the associated request method. HEAD, 1xx, 204, and 304 end at the header boundary; successful CONNECT enters a tunnel boundary. Other responses use TE, CL, or close-delimiting. | No-body/CONNECT tests. Parser callers must keep the request method stable. Both private exchanges currently pass `GET` rather than deriving method context from the serialized request; they are only valid for their GET proof. HEAD/304 metadata constraints cannot be fully checked without corresponding representation context. 205 is not a special framing case in RFC 9112, but RFC 9110 forbids a server from generating content for it; this parser currently exposes a declared 205 body rather than enforcing that semantic restriction. |
| Informational responses | The parser marks non-101 1xx as informational, completes that individual message, and supports `reset_message()` while retaining following bytes. 101 is marked as a terminal tunnel transition. | Parser test manually resets after coalesced 100 then 200. **Composition defect:** both private exchange drivers treat the first `message_complete` as the final response. A coalesced `100 Continue` + `200 OK` is rejected as surplus; if separated across reads, 100 may be returned as the exchange result. 101 is also not supported as an upgrade, but the exchange can treat it as a completed response. The smallest repair is for each single-request exchange to consume/reset any non-101 informational response and continue to the final response; explicitly reject/close on 101 until upgrade support exists. Add TCP and TLS regressions before using either driver beyond the current simple-GET proof. |
| EOF and truncation | Fixed-length/chunked premature EOF fails. Parser close-delimited responses complete only on explicit EOF. Extra bytes remain staged for parser reset; the one-response exchange rejects surplus bytes rather than interpreting them as another response. | EOF/surplus/parser-sticky-error corpus. Rejecting surplus for a single exchange is conservative and avoids treating bytes as another response. |
| TCP vs TLS | The same parser consumes byte spans, independent of transport. Plain TCP private exchange can signal EOF to the parser. HTTPS requires explicit response framing and rejects close-delimited responses; TLS truncation/failure is not converted into HTTP EOF. | P2B/P3B loopback tests cover fragmented fixed-length response, clean close, fixed-length HTTP truncation after TLS close_notify, and HTTPS close-delimited rejection. |
| Bounds and serialization | Parser bounds start lines (8 KiB), header section (32 KiB/100 fields), chunk lines (4 KiB), chunk metadata (64 KiB), trailers (8 KiB/32 fields), and staged bytes (64 KiB). It streams decoded body data into caller storage. Serializers validate tokens/field values, Host, framing exclusivity and bounded output; chunk serialization is bounded. | Constructor and limit/serialization tests. Parser body storage is caller-owned and therefore only bounded if callers provide bounded destinations; private exchanges cap accumulated response bodies at 64 KiB. |
| Request serialization semantics | The request serializer checks method/target syntax, exactly one valid Host, framing conflicts, field syntax and output bounds. | It does **not** compare Host with the authority in an absolute-form target. For example, it can emit `GET http://origin.example/ HTTP/1.1` with `Host: other.example`, contrary to RFC 9112's client requirement. Add authority-consistency validation (including the applicable CONNECT form) before using it as a general request serializer. |

The response-selection, HTTP/1.0 Host, 205-content, and outgoing-authority
findings above are source-confirmed, not runtime failures observed in the
current GET-only integration tests. They are the minimum protocol corrections
required before broadening private exchange coverage or defining a public API.

## H0-P4B correctness dispositions

These are private-prototype policies; they do not establish or freeze a public
HTTP API.

- A private single-request exchange consumes non-101 informational responses
  and continues to the final response. Parser reset preserves coalesced bytes;
  the parser's staged-input limit remains in force, and the exchange retains
  only its existing bounded body storage. An exchange ending after only
  informational responses fails as an incomplete response.
- `101 Switching Protocols` and successful `CONNECT` are rejected as
  unsupported protocol transitions. No upgrade or tunnel bytes are consumed
  as an ordinary response body.
- Each private exchange validates exactly one complete serialized request
  before acceptance and retains its parsed method from those same bytes. That
  method controls response framing, including HEAD. The private exchange
  rejects successful CONNECT instead of returning tunnel state.
- Every present request Host is syntax-validated for HTTP/1.0 and HTTP/1.1;
  HTTP/1.0 may omit it, while HTTP/1.1 requires exactly one. Duplicate fields
  are rejected. An empty value remains syntactically accepted by the parser.
- A 205 response is not treated as 204. Explicit `Content-Length: 0`, an empty
  chunked body, or an empty close-delimited representation are accepted. A
  nonzero Content-Length or any received content octet is rejected with the
  private `invalid_response_semantics` parse error. HTTPS exchanges continue
  to reject close-delimited framing regardless of status.
- The private request serializer checks absolute-form `http`/`https` target
  authority against the single Host field: host comparison is ASCII
  case-insensitive, explicit ports must both be present and numerically equal,
  and bracketed IPv6 literals are compared without exposing them to URL
  routing. Percent-escaped names, zone identifiers, userinfo, and other
  unsupported equivalence forms are conservatively rejected or compared
  byte-for-byte; output remains unchanged on failure.

Regression evidence is in `tests/http1_protocol_test.cpp`,
`tests/private_exchange_test.cpp`, and
`tests/private_tls_exchange_test.cpp`. These tests exercise parser semantics,
TCP and authenticated TLS response selection, both provider modes, unsupported
transitions, and method-dependent framing. They do not establish cross-platform
qualification or a public HTTP contract.

## H0-P4 security threat checklist disposition

This section follows the original H0-P4 acceptance checklist. “Validated”
means covered by source inspection and the named deterministic tests, not a
general security certification. The HTTP/1.1 P4B repairs are incorporated;
the preceding P4A table remains historical evidence and its explicitly marked
findings are superseded by the P4B dispositions above.

| Original threat | Current protection and evidence | Disposition and residual risk |
| --- | --- | --- |
| Request smuggling / ambiguous framing | Checked decimal Content-Length; equal duplicate/comma-list values only; conflicting values, TE+CL, unsupported transfer codings and malformed chunking fail terminally. `http1_protocol_test.cpp` covers these cases. | Mitigated for this parser subset. No downstream proxy/parser differential test or external fuzz qualification. Caller must close on parser error. |
| Response splitting / field injection | Octet-based syntax validation rejects CR/LF/control injection in start lines, names and values; serializers preflight and preserve prior output on rejection. Parser and serializer regressions cover malformed fields and injected output. | Validated for covered syntax and private serializer paths; no public serialization contract exists. |
| Truncated fixed/chunked messages | EOF before declared length or terminal chunk yields sticky `truncated_message`; prefix-boundary tests exercise truncation. TCP exchange tests cover short response; HTTPS tests distinguish TLS failure/close from HTTP completion. | Mitigated. HTTPS close-delimited framing remains unsupported; no connection reuse follows an incomplete exchange. |
| Malformed headers, line endings, folding and trailers | Strict CRLF and field-token checks, request obs-fold rejection, response obs-fold normalization, trailer deny list and bounded trailer parser. Malformed trailer-name/line-ending regressions were added. | Mitigated within parser subset. Arbitrary trailers remain separate and must not be merged by callers; trailer policy is intentionally conservative. |
| Conflicting Host / target authority | Duplicate and invalid present Host rejected, HTTP/1.1 requires one, HTTP/1.0 may omit it; serializer checks absolute-form target authority against Host. P4B parser/serializer tests cover these. | Mitigated for accepted syntax. Incoming absolute-form authority is not cross-checked against Host because an HTTP recipient uses the request-target authority under RFC 9112; no proxy behavior is implemented. |
| Unsupported upgrade/tunnel transitions | Parser recognizes the tunnel boundary; private exchanges explicitly reject 101 and successful CONNECT before returning an ordinary response. TCP/TLS exchange tests cover both provider modes. | Explicitly unsupported and safely rejected; no upgrade or tunnel API. |
| Surplus bytes / persistent-connection resynchronization | Parser preserves surplus only for an explicit subsequent parse after a complete message; single-exchange drivers reject surplus after the final response and retire the connection. | Persistent reuse/resynchronization is unsupported. There is no multi-exchange reuse contract or test and no bytes are reinterpreted as another response by the private exchange. |
| Resource exhaustion from parser-controlled sizes | Start line, header bytes/count, chunk line/metadata, trailer bytes/count and staged input have finite defaults; arithmetic is checked. Private response bodies are capped at 64 KiB. Tests exercise each parser limit and oversized serialization. | Bounded for an individual message/exchange. These are private defaults; parser body output is caller-owned and its size is controlled by the caller. This is not a process-wide memory budget. |
| Repeated informational responses / no-progress loops | The parser corpus verifies 128 coalesced informational messages make progress. Both private exchange drivers enforce an exchange-local limit of 128 (inclusive), tested at and above the boundary; fragmented/coalesced exchange cases and informational-only termination are covered. Each exchange also has one absolute 30-second default deadline, shortened by an earlier caller deadline. | The private prototype bounds informational-response count and individual exchange lifetime; it closes on overflow/timeout and does not reset the count across parser resets. Deadline observation is best-effort, not hard real-time. It is not a process-wide quota or aggregate memory budget. |
| Error-path ownership and cancellation | Parent exchanges retain leases/channels, buffers and started child operations until terminal retirement; Network/TLS tests cover cancellation/deadline, provider accounting, cooperative affinity and exactly-once completion. The HTTPS hang investigation proved and fixed a background startup lock inversion by queueing descriptor arming and initial I/O on the provider; full Debug, repeated HTTPS, and focused ASan/UBSan evidence is recorded in the implementation plan. | Linux private lifecycle evidence only. No ThreadSanitizer or cross-platform qualification. LeakSanitizer was not explicitly configured as a separate gate; no leak-specific conclusion is made here. |

One additional source-confirmed defect was found during this review: response
framing honored the no-body rule for 1xx/204 but accepted their forbidden
`Content-Length` metadata, including zero. It now fails with
`invalid_response_semantics` for any Content-Length field on 1xx or 204;
HEAD/304 metadata and the successful-CONNECT framing exception remain
unchanged. RFC 9110 §8.6 prohibits senders from including Content-Length on
1xx/204; RFC 9112 §6.3 still defines those responses as ending at the header
boundary. Regression cases cover 100, 103 and 204 with zero and nonzero
lengths, while preserving 205 zero-length behavior.

## 6. Proposed protocol and security invariants

The following are proposals, not established implementation guarantees.

| Invariant | Why and enforcement | Concrete test |
| --- | --- | --- |
| One authoritative framing decision per message | RFC 9112 framing rules are implemented once; conflicting Content-Length, Transfer-Encoding plus Content-Length, invalid chunk coding, and ambiguous syntax are rejected/handled exactly as specified before reuse. | Table-driven parser tests with conflicting/duplicate lengths, TE/CL combinations, invalid chunk extensions/terminators, then assert no next message is parsed from ambiguous bytes. |
| Bounded parser and body memory | Header bytes/count, line length, chunk metadata, and internal buffers have finite configured/default limits; body streaming does not imply body accumulation. | Fill each bound and verify deterministic limit error/backpressure with no unbounded allocation or byte loss. |
| No bytes cross message boundaries | Parser retains surplus bytes after one message and attributes them only to the next valid message on the same connection; body completion follows framing, not a read boundary. | Concatenate pipelined messages and split every delimiter across reads. |
| Truncation is not success | Fixed-length or chunked bodies ending early fail distinctly; close-delimited responses are only accepted where protocol semantics permit. | EOF before declared length/final chunk; valid close-delimited response. |
| Persistence requires known alignment | Reuse only after the request/response body is fully consumed or safely drained and parser is at a valid next-message boundary. Close after ambiguous or interrupted framing. | Cancellation at every body phase; attempt reuse only for fully drained aligned connection. |
| Streaming is backpressured | Body sink/source advances incrementally; bounded queues stop further reads/writes rather than dropping bytes. | Slow consumer/producer under small limits; verify bounded memory and byte order. |
| HTTP does not alter transport ownership | One layer exclusively owns a connection for an exchange/session; TLS channel's exclusive Network lease is not shared with raw TCP or another HTTP operation. | Competing-operation/close tests, provider accounting through terminal children. |
| HTTPS authenticates or fails | Use TLS's configured trust and reference identity; do not expose application response data before handshake success and do not downgrade. | Trusted/untrusted/mismatched identity and unavailable TLS target tests. |
| Cancellation/deadline has one terminal outcome | Pass Standard token/Deadline to Network-backed operation; retire started children before parent completion; do not claim rollback after partial transmission. | Race cancellation/deadline with native readiness; assert one stable completion and safe connection invalidation. |
| Cooperative mode remains cooperative | All progress is caused by Network provider polling; HTTP adds no worker or hidden runtime. | No-poll test makes no progress; owner-thread polling completes request/response. |

### Request-smuggling and malformed-message policy

The implementation must explicitly define and test every framing combination
recognized by RFC 9112, including duplicate or comma-combined Content-Length,
Transfer-Encoding precedence/ordering, invalid chunk framing, whitespace and
field syntax, and parser behavior after an error. It must not normalize an
ambiguous message in a way that lets a downstream peer parse a different
boundary. The safe default for an unresolvable framing ambiguity is reject and
close the connection. Whether RFC-required recipient recovery cases can be
accepted must be decided with a test proving exactly one interpretation.

Header names are case-insensitive for semantics while preserving field values
and order where needed. Do not silently combine fields whose semantics forbid
combination. Parsing remains octet-based; no locale-dependent text decoding
is applied to protocol syntax.

## 7. Supported first-release scope and exclusions

Proposed initial scope: HTTP/1.1 client and server message exchange, bounded
headers, incremental fixed-length/chunked body transfer, connection close and
carefully qualified persistence, Standard cancellation/deadline integration,
Network background/cooperative progression, and optional HTTPS only after
TLS is independently qualified and packaged.

Explicitly out of scope absent a separate foundational decision: HTTP/2,
HTTP/3, WebSocket, CONNECT tunneling/proxy support, automatic redirect/retry,
cookie jars, caching, authentication systems, routing, middleware, JSON,
templates, compression/content-coding conveniences, and application runtime.

## 8. Unresolved decisions

1. What exact request/response and body source/sink API keeps the common case
   simple while representing unknown-length streaming correctly?
2. Does v1 expose connection persistence/pipelining, or default to one
   exchange then close until request sequencing and cancellation recovery are
   fully proven? Pipelining should not be added for symmetry.
3. Which RFC recovery cases are supported versus rejected-and-closed, with a
   precise anti-smuggling rationale?
4. What default header/body limits are safe, portable, and adjustable without
   turning configuration into protocol internals?
5. What narrow private Network operation-composition seam is supported across
   domain libraries, including cooperative progress and install-tree builds?
6. How can optional TLS composition work without making stable plaintext
   HTTP depend on the experimental TLS target or OpenSSL?
7. Which server accept/dispatch ownership surface avoids implicitly adding a
   server runtime or unbounded connection admission?
8. Which platforms and toolchains can satisfy strict parser, transport,
   cancellation, and optional-TLS conformance?

These decisions remain open; this H0 document does not freeze them.
