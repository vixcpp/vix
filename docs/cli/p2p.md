# vix p2p

`vix p2p` runs a peer-to-peer node.

Use it when you want to start a local P2P node, connect it to another peer, enable discovery, or test peer communication from the CLI.

## Usage

```bash
vix p2p --id <node_id> --listen <port> [options]
```

## Required options

| Option | Description |
|--------|-------------|
| `--id <node_id>` | Unique node identifier |
| `--listen <port>` | TCP listen port |

## Basic usage

```bash
# Start node A
vix p2p --id A --listen 9001

# Start node B and connect to node A
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

## Connection options

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001 --connect-delay 1000
vix p2p --id B --listen 9002 --no-connect
```

## Auto-stop

```bash
vix p2p --id A --listen 9001 --run 10
```

## Discovery

```bash
vix p2p --id A --listen 9001 --discovery on --disc-port 37020
vix p2p --id A --listen 9001 --discovery off
vix p2p --id A --listen 9001 --disc-mode multicast
vix p2p --id A --listen 9001 --disc-interval 2000
```

## Bootstrap registry

```bash
vix p2p --id A \
  --listen 9001 \
  --bootstrap on \
  --registry http://127.0.0.1:8080 \
  --boot-interval 15 \
  --announce on
```

## Logging

```bash
vix p2p --id A --listen 9001 --log-level debug
```

Supported levels: `trace`, `debug`, `info`, `warn`, `error`, `critical`, `off`

## Core options

| Option | Description |
|--------|-------------|
| `--id <node_id>` | Unique node identifier |
| `--listen <port>` | TCP listen port |
| `--connect <host:port>` | Connect to a peer on startup |
| `--connect-delay <ms>` | Delay before connecting |
| `--run <seconds>` | Auto-stop after N seconds |
| `--stats-every <ms>` | Stats interval. Default is `1000` |
| `--tui <on\|off>` | Live stats display |
| `--quiet` | Print only final stats |
| `--no-connect` | Disable auto connect |

## Discovery options

| Option | Description |
|--------|-------------|
| `--discovery <on\|off>` | Enable/disable peer discovery |
| `--disc-port <port>` | Discovery port. Default is `37020` |
| `--disc-mode <mode>` | `broadcast` or `multicast`. Default is `broadcast` |
| `--disc-interval <ms>` | Discovery interval. Default is `2000` |

## Bootstrap options

| Option | Description |
|--------|-------------|
| `--bootstrap <on\|off>` | Enable/disable registry bootstrap |
| `--registry <url>` | Registry endpoint |
| `--boot-interval <sec>` | Refresh interval. Default is `15` |
| `--announce <on\|off>` | Announce node to registry |

## Common workflows

```bash
# Run one node
vix p2p --id A --listen 9001

# Run two local nodes (two terminals)
vix p2p --id A --listen 9001
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001

# Run a short demo
vix p2p --id A --listen 9001 --run 10

# Run quietly in a script
vix p2p --id A --listen 9001 --run 10 --quiet

# Use discovery
vix p2p --id A --listen 9001 --discovery on --disc-port 37020
vix p2p --id B --listen 9002 --discovery on --disc-port 37020
```

## Common mistakes

### Reusing the same port or node id

```bash
# Each node needs its own port and id
vix p2p --id A --listen 9001
vix p2p --id B --listen 9002
```

### Connecting before the peer is ready

Start node A first, then node B. Or use `--connect-delay 1000`.

### Using different discovery ports

Nodes must share the same `--disc-port` to discover each other.

### Enabling bootstrap without a registry endpoint

```bash
vix p2p --id A \
  --listen 9001 \
  --bootstrap on \
  --registry http://127.0.0.1:8080
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix run` | Run a project or C++ file |
| `vix dev` | Run an app in development mode |
| `vix check` | Validate project behavior |
| `vix info` | Inspect Vix paths and local state |
| `vix doctor` | Check the local environment |

## Next step

Continue with environment information.

[Open the vix info guide](/cli/info)
