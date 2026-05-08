# P2P

Run two Vix P2P nodes and connect them together.

```txt
node A ↔ node B
```

## What you will build

```txt
Terminal 1 → node-a listens on port 9101
Terminal 2 → node-b connects to node-a
```

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/p2p
cd ~/tmp/vix-examples/p2p
touch main.cpp
```

## Full code

```cpp
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vix/p2p.hpp>

namespace
{
  constexpr std::uint16_t kServerPort = 9101;

  static void sleep_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

  static std::shared_ptr<vix::p2p::Node> make_node(const std::string &node_id, std::uint16_t port)
  {
    vix::p2p::NodeConfig cfg;
    cfg.node_id = node_id;
    cfg.listen_port = port;
    cfg.on_log = [](std::string_view line) { std::cout << line << "\n"; };
    return vix::p2p::make_tcp_node(cfg);
  }

  static void print_stats(const std::string &label, const vix::p2p::NodeStats &stats)
  {
    std::cout << label
              << " peers_total=" << stats.peers_total
              << " peers_connected=" << stats.peers_connected
              << " handshakes_started=" << stats.handshakes_started
              << " handshakes_completed=" << stats.handshakes_completed
              << " connect_attempts=" << stats.connect_attempts
              << " connect_failures=" << stats.connect_failures << "\n";
  }

  static void run_server()
  {
    auto node = make_node("node-a", kServerPort);
    std::cout << "[server] starting node-a on tcp://127.0.0.1:" << kServerPort << "\n";
    node->start();

    for (int i = 0; i < 20; ++i) {
      print_stats("[server]", node->stats());
      sleep_ms(1000);
    }

    node->stop();
    std::cout << "[server] stopped\n";
  }

  static void run_client(const std::string &host, std::uint16_t port)
  {
    auto node = make_node("node-b", 0);
    vix::p2p::P2PRuntime runtime(node);
    std::cout << "[client] starting node-b\n";
    runtime.start();

    vix::p2p::PeerEndpoint endpoint;
    endpoint.host = host; endpoint.port = port; endpoint.scheme = "tcp";
    const bool started = runtime.connect(endpoint);
    std::cout << "[client] connect started: " << (started ? "true" : "false") << "\n";

    for (int i = 0; i < 15; ++i) {
      print_stats("[client]", runtime.stats());
      sleep_ms(1000);
    }

    runtime.stop();
    std::cout << "[client] stopped\n";
  }
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cout << "usage:\n  vix run main.cpp --run server\n  vix run main.cpp --run client 127.0.0.1 9101\n";
    return 0;
  }

  const std::string mode = argv[1];
  if (mode == "server") {
    run_server();
    return 0;
  }

  if (mode == "client")
  {
    const std::string host = argc >= 3 ? argv[2] : "127.0.0.1";
    const std::uint16_t port = argc >= 4 ? static_cast<std::uint16_t>(std::stoi(argv[3])) : kServerPort;
    run_client(host, port);
    return 0;
  }

  std::cout << "unknown mode: " << mode << "\n";
  return 1;
}
```

## Run

```bash
# Terminal 1
vix run main.cpp --run server

# Terminal 2
vix run main.cpp --run client 127.0.0.1 9101
```

> **Important:** use `--run`, not `--`. `--run` passes arguments to your program, `--` passes compiler/linker flags.

## UDP discovery

```cpp
vix::p2p::DiscoveryConfig cfg;
cfg.self_node_id = "node-a";
cfg.self_tcp_port = 9201;
cfg.discovery_port = 37020;
cfg.mode = vix::p2p::DiscoveryMode::Broadcast;
cfg.announce_interval_ms = 1000;
cfg.seen_ttl_ms = 15000;

auto discovery = vix::p2p::make_udp_discovery(cfg,[](const vix::p2p::DiscoveryAnnouncement &peer){
  std::cout << "discovered " << peer.node_id << " at " << peer.host << ":" << peer.port << "\n";
});

discovery->start();
auto peers = discovery->snapshot();
discovery->stop();
```

## Bootstrap registry

```cpp
vix::p2p::BootstrapConfig cfg;
cfg.registry_url = "http://127.0.0.1:8080/peers";
cfg.poll_interval_ms = 2000;
// Registry returns: { "peers": [{ "host": "...", "tcp_port": 9301, "node_id": "..." }] }
```

## P2P HTTP control API

```cpp
#include <vix/p2p_http.hpp>

vix::p2p_http::P2PHttpOptions options;
options.prefix = "/p2p";
options.enable_ping = options.enable_status = options.enable_peers = true;
vix::p2p_http::registerRoutes(app, runtime, options);
// Routes: GET /p2p/ping, GET /p2p/status, GET /p2p/peers, POST /p2p/connect
```

## P2P and Sync together

```txt
local write → WAL → outbox → P2P message (WalPush) → peer → WalAck → convergence
```

## Common mistakes

```bash
# Wrong order — start server first
vix run main.cpp --run client 127.0.0.1 9101   # nothing to connect to

# Correct
vix run main.cpp --run server                  # terminal 1
vix run main.cpp --run client 127.0.0.1 9101   # terminal 2

# Wrong — -- is for compiler flags
vix run main.cpp -- client 127.0.0.1 9101
# Correct
vix run main.cpp --run client 127.0.0.1 9101
```

```cpp
// Wrong — same node id
make_node("node-a", 9101); make_node("node-a", 9102);  // conflict

// Correct
make_node("node-a", 9101); make_node("node-b", 9102);

// Wrong — forgetting runtime.start()
runtime.connect(endpoint);  // before start()
// Correct
runtime.start();
runtime.connect(endpoint);
```

## What you should remember

```cpp
vix::p2p::NodeConfig cfg;
cfg.node_id = "node-a";
cfg.listen_port = 9101;
auto node = vix::p2p::make_tcp_node(cfg);
node->start();

vix::p2p::P2PRuntime runtime(node);
runtime.start();
vix::p2p::PeerEndpoint endpoint;
endpoint.host = "127.0.0.1";
endpoint.port = 9101;
endpoint.scheme = "tcp";
runtime.connect(endpoint);
runtime.stop();
```

The core idea: **P2P gives Vix node-to-node communication for distributed and offline-first systems.**

Next: [Production App](/examples/production-app)
