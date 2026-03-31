# vix p2p

Run a Vix P2P node (TCP + discovery + optional bootstrap).

---

## Usage

```bash
vix p2p --id <node_id> --listen <port> [options]
```

---

## Description

`vix p2p` starts a peer-to-peer node with:

- TCP transport
- Optional UDP discovery
- Optional HTTP bootstrap registry
- Live stats (TUI mode)
- Structured logging

Designed for testing distributed networking locally or across nodes.

---

## Core Options

```bash
--id <node_id>                 Node id (string, required)
--listen <port>                TCP listen port (required)
--connect <host:port>          Connect to peer after start
--connect-delay <ms>           Delay before connect()
--run <seconds>                Auto-stop after N seconds
--stats-every <ms>             Stats interval (default: 1000)
--tui <on|off>                 Single-line live stats (default: auto on TTY)
--quiet                        Print only final stats
--no-connect                   Disable auto connect callbacks
```

---

## Discovery (UDP)

```bash
--discovery <on|off>           Default: on
--disc-port <port>             Default: 37020
--disc-mode <broadcast|multicast>  Default: broadcast
--disc-interval <ms>           Default: 2000
```

Allows peers on the same network to auto-discover each other.

---

## Bootstrap (HTTP Registry)

```bash
--bootstrap <on|off>           Default: off
--registry <url>               Default: http://127.0.0.1:8080/p2p/v1
--boot-interval <seconds>      Default: 15
--announce <on|off>            Default: on
```

Used to:

- Register node presence
- Discover remote peers via registry endpoint

---

## Logging

```bash
--log-level <level>
```

Levels:

```
trace | debug | info | warn | error | critical | off
```

---

## Examples

Single node:

```bash
vix p2p --id A --listen 9001
```

Connect two peers:

```bash
vix p2p --id A --listen 9001
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

Run for 10 seconds:

```bash
vix p2p --id A --listen 9001 --run 10
```

Disable discovery:

```bash
vix p2p --id A --listen 9001 --discovery off
```

Enable bootstrap:

```bash
vix p2p --id A --listen 9001 --bootstrap on
```

---

## Typical Workflow

Open two terminals:

Terminal 1:
```bash
vix p2p --id A --listen 9001
```

Terminal 2:
```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

---

`vix p2p` enables distributed experimentation without external dependencies.

