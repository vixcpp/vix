# P2P API

The P2P API is the distributed networking API of Vix. It provides building blocks for nodes that need to discover peers, connect, exchange protocol messages, and support offline-first replication.

```txt
peer discovery — peer connection — message framing — handshake — routing — WAL replication — HTTP control
```

## Public headers

```cpp
#include <vix/p2p.hpp>          // core P2P runtime, protocol, routing, discovery
#include <vix/p2p_http.hpp>     // HTTP control routes for P2P runtime
```

Lower-level headers when needed:

```cpp
#include <vix/p2p/Node.hpp>
#include <vix/p2p/P2P.hpp>
#include <vix/p2p/Discovery.hpp>
#include <vix/p2p/Bootstrap.hpp>
#include <vix/p2p/Router.hpp>
#include <vix/p2p/Protocol.hpp>
#include <vix/p2p/Framing.hpp>
#include <vix/p2p/messages/Envelope.hpp>
#include <vix/p2p/messages/Pack.hpp>
#include <vix/p2p/messages/Dispatch.hpp>
```

## Architecture

```txt
Discovery → Peer endpoint → Transport → Framing → Envelope → Message dispatch → Runtime state
```

For offline-first replication:

```txt
local write → WAL → WalPush → peer applies → WalAck → convergence
```

## Main namespaces

| Namespace | Purpose |
|-----------|---------|
| `vix::p2p` | Core P2P runtime, protocol, routing, discovery |
| `vix::p2p::msg` | Typed P2P protocol messages |
| `vix::p2p::pack` | Envelope packing helpers |
| `vix::p2p::framing` | Framing implementations |
| `vix::p2p_http` | HTTP control routes for P2P runtime |

## Runtime API

### NodeConfig

```cpp
vix::p2p::NodeConfig cfg;
cfg.node_id = "node-a";        // stable unique identity
cfg.listen_port = 9101;        // TCP listen port

cfg.on_log = [](std::string_view line) {
  std::cout << line << "\n";
};
```

### Node

```cpp
auto node = vix::p2p::make_tcp_node(cfg);
node->start();   // begin listening
node->wait();    // block until stopped
node->stop();    // shutdown
auto stats = node->stats();
```

### P2PRuntime

```cpp
vix::p2p::P2PRuntime runtime(node);
runtime.start();

// Connect to a peer
vix::p2p::PeerEndpoint endpoint;
endpoint.host = "127.0.0.1";
endpoint.port = 9101;
endpoint.scheme = "tcp";
const bool started = runtime.connect(endpoint);

auto stats = runtime.stats();
runtime.stop();
```

### NodeStats fields

| Field | Meaning |
|-------|---------|
| `peers_total` | Number of known peers |
| `peers_connected` | Number of connected peers |
| `handshakes_started` | Started handshakes |
| `handshakes_completed` | Completed handshakes |
| `connect_attempts` | Connection attempts |
| `connect_failures` | Failed connections |
| `backoff_skips` | Skipped due to backoff |

## Minimal node

```cpp
#include <vix/p2p.hpp>

auto node = vix::p2p::make_tcp_node(cfg);
node->start();
std::cout << "listening on port 9101\n";
node->wait();
```

## Discovery API

```cpp
vix::p2p::DiscoveryConfig cfg;
cfg.self_node_id = "node-a";
cfg.self_tcp_port = 9201;
cfg.discovery_port = 37020;
cfg.mode = vix::p2p::DiscoveryMode::Broadcast;
cfg.announce_interval_ms = 1000;
cfg.seen_ttl_ms = 15000;
cfg.connect_cooldown_ms = 4000;

auto on_peer = [](const vix::p2p::DiscoveryAnnouncement &peer)
{
  std::cout << "discovered " << peer.node_id << " at " << peer.host << ":" << peer.port << "\n";
};

auto discovery = vix::p2p::make_udp_discovery(cfg, on_peer);
discovery->start();
const auto peers = discovery->snapshot();
discovery->stop();
```

### Discovery message

```cpp
vix::p2p::msg::DiscoveryAnnounce announce;
announce.node_id = "node-a";
announce.tcp_port = 9001;
announce.ts_ms = 1710000000000ULL;
announce.capabilities["proto"] = "1.0";

const std::string json = announce.to_json();
const auto parsed = vix::p2p::msg::DiscoveryAnnounce::from_json(json);
```

## Bootstrap API

```cpp
vix::p2p::BootstrapConfig cfg;
cfg.self_node_id = "bootstrap-client";
cfg.self_tcp_port = 9300;
cfg.registry_url = "http://127.0.0.1:8080/peers";
cfg.mode = vix::p2p::BootstrapMode::PullOnly;
cfg.poll_interval_ms = 2000;

auto bootstrap = vix::p2p::make_http_bootstrap(cfg, on_peer);
bootstrap->start();
const auto peers = bootstrap->snapshot();
bootstrap->stop();
```

Registry response shape:

```json
{ "peers": [{ "host": "127.0.0.1", "tcp_port": 9301, "node_id": "node-x" }] }
```

## Router API

```cpp
vix::p2p::MemoryRouter router;
router.upsert_route("node-b", vix::p2p::Route{"edge-1", false, 8});  // next_hop, via_relay, ttl
router.upsert_route("node-c", vix::p2p::Route{"relay-7", true, 4});

const auto route = router.resolve("node-b");
if (route)
  std::cout << "next_hop=" << route->next_hop << "\n";

router.remove_route("node-c");
```

## Protocol API

### Envelope

```cpp
vix::p2p::msg::Ping ping;
ping.nonce = 42;

vix::p2p::Envelope envelope = vix::p2p::pack::make_envelope(
    vix::p2p::MessageType::Ping, ping);

const std::vector<std::uint8_t> bytes = envelope.encode();
vix::p2p::Envelope decoded = vix::p2p::Envelope::decode_or_throw(bytes);

std::cout << vix::p2p::to_string(envelope.type) << "\n";
bool encrypted = vix::p2p::has_flag(envelope.flags, vix::p2p::EnvelopeFlag::Encrypted);
```

### Framing

TCP is a byte stream — framing marks where each message begins and ends.

```cpp
vix::p2p::framing::LengthPrefixVarint framer;

vix::p2p::Frame frame = framer.encode(envelope_bytes);
vix::p2p::FrameDecodeResult result = framer.decode(frame.bytes);

if (!result.frames.empty())
{
  // result.frames.front().bytes — first complete frame
  // result.remaining             — leftover bytes for next read
}
```

### Dispatch

```cpp
vix::p2p::msg::AnyMessage any = vix::p2p::msg::decode_payload_or_throw(
    vix::p2p::MessageType::Ping, payload);

if (std::holds_alternative<vix::p2p::msg::Ping>(any))
{
  const auto &ping = std::get<vix::p2p::msg::Ping>(any);
  std::cout << "nonce=" << ping.nonce << "\n";
}
```

## Message types

| Message | Purpose |
|---------|---------|
| `Hello` | First handshake message |
| `HelloAck` | Handshake response |
| `HelloFinish` | Final handshake message |
| `Ping` / `Pong` | Liveness check |
| `WalPush` | Push WAL bytes to a peer |
| `WalAck` | Acknowledge applied WAL sequence |
| `OutboxPull` | Ask peer for pending outbox items |

### Handshake

```cpp
// A → B → A
vix::p2p::msg::Hello hello;
hello.nonce_a = 1001; hello.node_id = "node-a";
hello.capabilities["proto"] = "1.0";

vix::p2p::msg::HelloAck ack;
ack.nonce_a = 1001; ack.nonce_b = 2002;

vix::p2p::msg::HelloFinish finish;
finish.nonce_a = 1001; finish.nonce_b = 2002; finish.signature = signature;

// Encode/decode all messages the same way
const auto bytes = hello.encode();
const auto decoded = vix::p2p::msg::Hello::decode_or_throw(bytes);
```

### WAL replication messages

```cpp
// Push WAL records to peer
vix::p2p::msg::WalPush push;
push.seq_begin = 10; push.seq_end = 12; push.wal_bytes = wal_bytes;

// Acknowledge what peer has applied
vix::p2p::msg::WalAck ack;
ack.last_applied_seq = 12;

// Ask peer for pending operations
vix::p2p::msg::OutboxPull pull;
pull.target_node_id = "node-b"; pull.max_items = 64;
```

## Crypto API

```cpp
// Secure envelope packing
vix::p2p::Envelope env = vix::p2p::pack::make_envelope_secure(
    vix::p2p::MessageType::Ping, plaintext, session_key, crypto, 1);

const auto aad = vix::p2p::pack::make_aad(env);
const auto decrypted = crypto.aead_decrypt(session_key, nonce, aad, env.payload, env.tag);

// NullCrypto for testing
vix::p2p::NullCrypto crypto;
```

## P2P HTTP API

```cpp
#include <vix/p2p_http.hpp>

vix::p2p_http::P2PHttpOptions options;
options.prefix = "/p2p";
options.enable_ping = true;
options.enable_status = true;
options.enable_peers = true;
options.enable_logs = true;
options.enable_live_logs = true;

vix::App app;
vix::p2p_http::registerRoutes(app, runtime, options);
// Don't forget on shutdown:
vix::p2p_http::shutdown_live_logs();
```

### HTTP control routes

| Route | Purpose |
|-------|---------|
| `GET /p2p/ping` | Smoke test |
| `GET /p2p/status` | Runtime stats |
| `POST /p2p/connect` | Connect to a peer |
| `GET /p2p/peers` | Known peers |
| `GET /p2p/logs` | In-memory log buffer |
| `POST /p2p/admin/hook` | Admin extension hook |

### Connect via HTTP

```bash
curl -X POST http://127.0.0.1:8080/p2p/connect \
  -H "content-type: application/json" \
  -d '{"host":"127.0.0.1","port":9101,"scheme":"tcp"}'
```

### Auth hooks

```cpp
// Preferred: middleware context
options.auth_ctx = [](vix::mw::Context &ctx) -> bool
{
  if (ctx.req().header("x-auth-token") == "secret")
    return true;

  ctx.res().status(401).json(vix::json::obj({
    {"ok", false},
    {"error", "unauthorized"}
  }));

  return false;
};

// Fallback: legacy hook
options.auth_legacy = [](vix::http::Request &req, vix::http::ResponseWrapper &res) -> bool
{
  if (req.header("x-auth-token") == "secret")
    return true;

  res.status(401).json(vix::json::obj({
    {"ok", false},
    {"error", "unauthorized"}
  }));

  return false;
};
```

### Custom log sink

```cpp
options.log_sink = [](std::string_view line) {
  std::cout << "[p2p_http] " << line << "\n";
};

vix::p2p_http::set_live_log_sink([](std::string line) {
  std::cout << "[live] " << line << "\n";
});

vix::p2p_http::shutdown_live_logs();  // always call on shutdown
```

## P2P CLI

```bash
vix p2p --id A --listen 9001
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
vix p2p --id A --listen 9001 --discovery on --disc-port 37020
vix p2p --id A --listen 9001 --bootstrap on --registry http://127.0.0.1:8080
```

## How P2P fits with Sync

```txt
local write → WAL → outbox → P2P message → peer → ack → convergence
```

`WalPush`, `WalAck`, `OutboxPull` are protocol messages designed for distributed replication.

## Common mistakes

### Reusing node id or listen port

Each node must have a unique `node_id` and its own `listen_port`.

### Connecting before peer is ready

Start the target node first, then connect from the second node.

### Forgetting runtime.start()

```cpp
runtime.start();   // must call before runtime.connect()
runtime.connect(endpoint);
```

### Exposing control routes without auth

Routes like `POST /p2p/connect`, `GET /p2p/peers`, `POST /p2p/admin/hook` must be protected in production.

### Confusing WebSocket and P2P

WebSocket → browser clients to a server. P2P → nodes to nodes.

## Production notes

- Use stable node ids
- Protect all P2P HTTP control routes
- Monitor `peers_connected`, handshake failures, `connect_failures`
- Treat P2P messages as untrusted input — validate every payload
- Keep discovery ports consistent across the fleet
- HTTP control routes can go through Nginx; P2P transport requires direct TCP reachability or a relay

## What you should remember

```cpp
// Runtime
vix::p2p::NodeConfig cfg;
cfg.node_id = "node-a";
cfg.listen_port = 9101;
auto node = vix::p2p::make_tcp_node(cfg);
vix::p2p::P2PRuntime runtime(node);
runtime.start();
runtime.connect(endpoint);
runtime.stop();

// Protocol flow
typed message → envelope → frame → transport → decode → dispatch

// Sync flow
WAL → WalPush → WalAck → convergence
```

The core idea: P2P is the node-to-node layer that makes Vix suitable for distributed and offline-first systems.
