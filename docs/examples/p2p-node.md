# P2P Node

This example shows how to start a minimal P2P node using the `vix::p2p` module.

What you get:

- TCP node (inbound + outbound)
- UDP discovery (LAN broadcast or multicast)
- Optional HTTP bootstrap (registry pull, optional announce)
- Secure handshake wiring (dev crypto by default)
- Ping/Pong heartbeats and stale peer cleanup

---

## Requirements

- C++20
- Vix.cpp installed
- `modules/p2p` enabled

---

## Minimal Example

Create `p2p_node.cpp`:

```cpp
#include <vix/p2p/P2P.hpp>
#include <vix/p2p/Node.hpp>
#include <vix/p2p/Discovery.hpp>
#include <vix/p2p/Bootstrap.hpp>
#include <vix/p2p/Router.hpp>
#include <vix/p2p/Crypto.hpp>

#include <iostream>
#include <memory>
#include <string>

static void log_line(std::string_view s)
{
  std::cerr << s << "\n";
}

int main(int argc, char** argv)
{
  using namespace vix::p2p;

  std::string nodeId = "node-A";
  std::uint16_t listenPort = 9001;
  std::string bootstrapUrl;

  if (argc >= 2) nodeId = argv[1];
  if (argc >= 3) listenPort = static_cast<std::uint16_t>(std::stoi(argv[2]));
  if (argc >= 4) bootstrapUrl = argv[3];

  NodeConfig cfg;
  cfg.node_id = nodeId;
  cfg.listen_port = listenPort;
  cfg.max_peers = 64;
  cfg.handshake_timeout_ms = 3000;
  cfg.on_log = log_line;

  auto node = make_tcp_node(std::move(cfg));
  node->set_router(std::make_shared<MemoryRouter>());
  node->set_crypto(std::make_shared<NullCrypto>());

  if (!bootstrapUrl.empty())
  {
    BootstrapConfig bcfg;
    bcfg.self_node_id = nodeId;
    bcfg.self_tcp_port = listenPort;
    bcfg.registry_url = bootstrapUrl;
    bcfg.mode = BootstrapMode::PullAndAnnounce;

    node->set_bootstrap(make_http_bootstrap(std::move(bcfg), nullptr));
  }

  auto runtime = std::make_shared<P2PRuntime>(node);
  runtime->start();

  std::cerr << "[p2p] started node_id=" << nodeId
            << " listen=" << listenPort << "\n";

  runtime->wait();
  return 0;
}
```

---

## Run

### Local LAN test

Terminal 1:

```bash
vix run p2p_node.cpp -- node-A 9001
```

Terminal 2:

```bash
vix run p2p_node.cpp -- node-B 9002
```

Nodes discover each other via UDP broadcast.

---

### With HTTP bootstrap registry

If a registry is running on port 8089:

```bash
vix run p2p_node.cpp -- node-A 9001 http://127.0.0.1:8089
vix run p2p_node.cpp -- node-B 9002 http://127.0.0.1:8089
```

---

## Notes

- Default crypto is `NullCrypto` (development only).
- Transport is TCP.
- Ping/Pong is automatic.
- For production, replace crypto with a secure implementation.

