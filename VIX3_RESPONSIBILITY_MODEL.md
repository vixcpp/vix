# VIX3 Responsibility Model

## Purpose

This document freezes the conceptual Vix3 responsibility model before restructuring begins. It defines Vix Platform, Vix Ecosystem, and the provider layers beneath Vix contracts.

## Non-goals

This document does not define final APIs, class names, source layout, repository layout, package layout, physical module moves, or implementation sequence. It is an architecture contract, not a migration plan.

Historical module names, current repository location, CMake optionality, and package-manager availability do not determine Vix3 placement.

## Definition of Vix

```text
Vix = Platform + Ecosystem
```

Platform defines Vix identity. Ecosystem builds on Platform. Providers implement or expose capabilities beneath Vix contracts.

## Platform

```text
Platform = Toolchain + Runtime
```

Platform contracts are reusable across Vix application domains. Platform does not depend on Ecosystem. Toolchain may consume Runtime capabilities.

## Toolchain

Toolchain owns the capabilities required to create, describe, resolve, build, test, diagnose, distribute, and operate Vix projects.

```text
Toolchain
├── CLI
├── project model
├── dependency management
├── package management
├── build engine
├── test orchestration
├── diagnostics
├── SDK management
└── distribution tooling
```

The current CLI provides direct evidence for each responsibility:

- Project model is used by init, run, build, check, and module flows through manifests, lockfiles, project resolution, dependency constraints, and module graphs.
- Dependency management is implemented by add, remove, install, and update flows with resolver and lockfile behavior.
- Package management owns registry, install, publish, unpublish, pack, and global-package workflows.
- Build engine behavior is represented by CLI build flows and the build-engine graph, planning, tool-discovery, and process-execution components.
- Test orchestration is represented by the CLI test flow and generated test/build integration.
- SDK management owns SDK profiles, upgrade, CMake package registration, and uninstall behavior.
- Distribution tooling owns packaging, publication, unpublication, and update workflows.
- Diagnostics owns doctor, compiler/build/runtime error classification, and diagnostic pipelines.

Toolchain consumes Runtime capabilities including filesystem, path, environment, process, JSON, crypto, and HTTP client.

## Runtime

Runtime provides cross-domain contracts and primitives used by Toolchain and Ecosystem.

```text
Runtime
├── Foundation
└── General Capabilities
```

Runtime is not defined by historical modules named `core`, `utils`, or `net`.

## Runtime Foundation

Runtime Foundation contains general primitives that establish common Vix behavior.

```text
Foundation
├── errors and result
├── path and filesystem
├── environment
├── time
├── logging
└── async, coroutines, and cancellation
```

Foundation contracts are independent of application protocols and domain packages.

## General Runtime Capabilities

General Runtime Capabilities are reusable, cross-domain services built on Runtime Foundation.

```text
General Capabilities
├── blocking and CPU offload
├── process
├── TCP, UDP, and DNS transport
├── TLS transport
├── JSON
├── crypto
└── HTTP client
```

HTTP client is a Platform Runtime capability. Vix Toolchain directly consumes it for upgrade, cloud, doctor, service, and health-check workflows.

## Ecosystem

Ecosystem contains application-facing, protocol-facing, and domain-oriented capabilities built on Platform.

```text
Ecosystem
├── HTTP server, routing, sessions, and OpenAPI
├── HTTP response caching and offline policy
├── durable outbox and delivery synchronization
├── database and ORM
├── local KV storage
├── WebSocket
├── realtime
├── P2P
├── UI
├── Game
└── Agent
```

Ecosystem does not define Platform identity.

## Providers

Providers are beneath Vix contracts.

```text
Providers
├── Build Providers
├── System Providers
└── Managed / External Implementations
```

### Build Providers

Build Providers expose build and tool-discovery capabilities.

```text
Build Providers
├── CMake
├── Ninja
├── GCC
├── Clang
├── MSVC
└── linker and tool discovery
```

### System Providers

System Providers are capabilities fundamentally supplied by the host operating system or native platform.

```text
System Providers
├── threads
├── clocks
├── filesystem APIs
├── process APIs
├── sockets
├── DNS
└── native OS APIs and frameworks
```

### Managed / External Implementations

Managed or External Implementations are replaceable libraries or Vix-managed engines implementing Vix contracts.

```text
Managed / External Implementations
├── Asio
├── OpenSSL
├── nlohmann-json
├── fmt and spdlog
├── SQLite, MySQL, and Redis client libraries
├── SDL, GTK, and WebKit
└── other replaceable implementation libraries
```

Asio, OpenSSL, nlohmann-json, SQLite, SDL, and GTK are not System Providers.

## Dependency direction

```text
Ecosystem packages
        ↓
Platform Toolchain and Runtime capabilities
        ↓
Platform Foundation contracts
        ↓
Provider adapters and managed implementations
        ↓
System Providers and host platform
```

Toolchain consumes Runtime capabilities. Platform does not depend on Ecosystem. Protocols consume transport capabilities rather than defining transport ownership.

## Execution model

The required execution model is:

```text
async I/O, coroutines, cancellation
        +
blocking and CPU offload
```

Async owns coroutine scheduling, cancellation, timers, TCP, UDP, DNS, and asynchronous I/O. Blocking and CPU work are isolated through async-owned offload facilities.

Game owns the demonstrated specialized job-execution requirement: worker jobs, priority, task handles, status, and optional cancellation. `threadpool` is not currently demonstrated as Platform identity.

The following historical public surfaces are not Platform requirements:

```text
core runtime work stealing
core runtime cooperative budgets
worker affinity
threadpool task groups
threadpool parallel algorithms
threadpool deadlines
threadpool periodic jobs
```

No production consumer establishes these as Platform identity.

## Transport model

The transport direction is:

```text
System sockets and DNS
        ↓
Vix async transport contracts
        ↓
TLS transport
        ↓
HTTP client · HTTP server · WebSocket · P2P
```

`async` owns Vix transport contracts for TCP streams, TCP listeners, UDP sockets, DNS resolution, cancellation, and coroutine-based asynchronous operations.

Asio is an implementation beneath Vix transport contracts. It is not a Vix public contract. P2P's current public Asio exposure is implementation/provider leakage.

The transport boundary must support demonstrated P2P requirements:

```text
serialized stream execution
UDP multicast and broadcast options
TCP half-close
connection endpoint metadata
```

## TLS model

TLS is a Runtime transport capability.

```text
TCP stream
    ↓
TLS stream
    ↓
protocol
```

The Vix TLS contract owns client-verification policy, server certificate and private-key policy, SNI behavior, handshake behavior, encrypted read and write, cancellation and timeout integration, and connection shutdown.

OpenSSL is the current TLS implementation/provider. OpenSSL types are not part of the conceptual Vix contract.

HTTP client and HTTP server use different role-specific TLS policy. Those policies do not invalidate one shared TLS transport boundary. P2P and WebSocket have no current source-level TLS transport implementation.

## Historical boundaries that do not survive unchanged

### `core`

`core` does not survive as one conceptual boundary.

```text
execution primitives
    → Runtime Foundation

HTTP server, routing, sessions, OpenAPI
    → Ecosystem HTTP server capability

view and template integration
    → Ecosystem application layer
```

### `utils`

`utils` does not survive as a named architectural concept.

```text
logging
    → logging

environment
    → environment and runtime configuration

result
    → errors and result

validation
    → validation, if retained

time
    → time

server presentation
    → HTTP/server developer experience

strings and scope guards
    → focused primitives or implementation details
```

Vix3 does not introduce another generic utility bucket.

### `net`

`net` does not survive as a Vix3 architectural concept.

```text
TCP, UDP, DNS
    → async transport contracts

HTTP client
    → HTTP client capability

CurlClient
    → historical duplicate implementation

NetworkProbe
    → connectivity/offline-policy consumer, if retained
```

### `cache`

Current cache is an HTTP-response and offline-policy contract.

```text
HTTP status, headers, body, GET keys, freshness
    → HTTP response caching

offline and network-error stale reuse
    → offline/network policy

memory, LRU, file persistence
    → managed storage implementations
```

Vix3 does not define a Platform generic cache contract without demonstrated independent consumers.

### `sync`

Current sync responsibility is durable outbox and delivery coordination. It is not a general synchronization primitive.

### `kv`

KV is an Ecosystem data capability unless future Platform dependency evidence changes. WAL, segments, compaction, snapshots, and storage format are managed implementation details.

## Stability model

Platform Foundation contracts are stable Vix identity contracts.

Platform General Capability contracts are stable where Toolchain or multiple cross-domain consumers require them.

Ecosystem contracts are stable within their capability domain but do not define Platform identity.

Provider interfaces and provider selection remain replaceable behind Vix contracts. Managed implementations do not become public contracts solely because they are currently the only implementation.

## Compatibility policy

Vix3 Platform starts clean. Vix2 compatibility is a removable migration facility outside Platform, not Vix3 architectural identity.

```text
Vix2 user code
        ↓
compatibility surface
        ↓
canonical Vix3 Platform or Ecosystem contracts
```

The reverse dependency is forbidden. New Vix3 APIs do not depend on compatibility APIs. Compatibility adapters may depend on canonical Platform or Ecosystem contracts, but canonical contracts do not depend on compatibility adapters.

Compatibility is preserved only when existing semantics can be reproduced without retaining ownership that Vix3 rejects. Compatibility surfaces are explicitly deprecated, removable, and are not part of the canonical Vix3 Platform stability contract. They may be removed at an appropriate future major-version boundary.

Source compatibility, behavior compatibility, and provider compatibility are distinct commitments. Source compatibility does not require preserving provider-native types or historical execution and lifecycle behavior. Provider-native APIs are not preserved when doing so compromises canonical Vix3 contracts.

### `core` compatibility

Historical HTTP application/server APIs, routing, sessions, and OpenAPI may be bridged temporarily where their semantics remain equivalent. Such a bridge delegates to the canonical Ecosystem HTTP-server capability and remains outside Platform.

`core` does not remain a canonical Vix3 boundary. Historical `core::runtime` and `RuntimeExecutor` are intentionally broken. Vix3 does not preserve the work-stealing, worker-affinity, or cooperative-budget scheduler contract solely for compatibility.

### `utils` compatibility

`vix::utils` is not a canonical Vix3 concept. Focused symbols for logging, environment, errors/result, time, and validation may receive temporary forwarding only where their semantics remain equivalent.

Vix3 does not preserve an aggregate utility layer and does not introduce a replacement generic utility bucket.

### `net` compatibility

Historical `vix::net::http` may receive a temporary compatibility facade over the canonical Platform Runtime HTTP-client capability. The facade is not the HTTP implementation and does not alter canonical ownership.

`NetworkProbe` may remain temporarily as a compatibility helper. Its final canonical ownership is unresolved and is determined by the retained connectivity/offline-policy use case. `net` does not survive merely to host it.

`CurlClient` is intentionally broken. Vix3 does not preserve the curl-process implementation as an HTTP provider.

### `threadpool` compatibility

Standalone `threadpool` is not Platform Runtime identity. If retained for migration, it exists outside Platform as an Ecosystem/support capability and remains removable. Game retains ownership of the demonstrated specialized job requirement unless broader cross-domain requirements emerge.

### `cache` compatibility

The current HTTP-shaped cache contract may remain as an Ecosystem compatibility or capability surface. Vix3 does not preserve or introduce a generic Platform cache promise.

### P2P compatibility

P2P protocol and data APIs are preserved where their semantics do not depend on Asio. Public Vix3 P2P APIs do not accept `asio::io_context` or `asio::ip::tcp::socket`.

P2P Asio APIs are intentionally source-breaking. A migration-only adapter may exist outside canonical P2P APIs only when separately justified. It does not make Asio part of the Vix3 public transport contract.

## Rules for deciding where a future capability belongs

A capability belongs in Platform Runtime Foundation when it is a general primitive required by multiple Runtime capabilities.

A capability belongs in Platform Runtime General Capabilities when it is cross-domain, reusable, and required by Toolchain or multiple Ecosystem domains.

A capability belongs in Toolchain when it creates, describes, resolves, builds, tests, diagnoses, distributes, or operates Vix projects.

A capability belongs in Ecosystem when it is application-oriented, protocol-oriented, or domain-specific.

A capability belongs beneath Vix as a System Provider when the host platform fundamentally supplies it.

A capability belongs beneath Vix as a Managed or External Implementation when it is a replaceable library or Vix-managed implementation.

## Explicit unresolved implementation questions

- Unresolved: whether core runtime's budget and work-stealing implementation remains useful as an internal execution strategy.
- Unresolved: whether Game job execution eventually shares a common execution implementation while retaining Game-specific semantics.
- Unresolved: the exact transport extensions required for P2P serialization, multicast/broadcast, half-close, and endpoint metadata.
- Unresolved: the final provider-neutral TLS transport shape for client and server policy.
- Unresolved: whether additional independent consumers justify a future generic cache contract.

## Invariants

- Platform equals Toolchain plus Runtime.
- Ecosystem builds on Platform.
- Platform does not depend on Ecosystem.
- Toolchain may depend on Runtime capabilities.
- Runtime owns Vix transport contracts.
- Protocol packages do not own independent public transport families.
- Asio is not a Vix public contract.
- OpenSSL is not a Vix public contract.
- External libraries are not System Providers unless they are host-platform capabilities.
- HTTP client is a Platform Runtime capability.
- HTTP server is an Ecosystem capability.
- `core`, `utils`, and `net` are not Vix3 architectural boundaries.
- Vix3 does not introduce a generic utility bucket.
- Vix3 does not introduce a Platform generic cache without demonstrated independent consumers.
- KV is an Ecosystem data capability unless Platform dependency evidence changes.
- Managed implementation selection remains separate from public Vix capability ownership.
