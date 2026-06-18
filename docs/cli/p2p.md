# vix p2p

`vix p2p` runs a peer-to-peer node from the CLI.

Use it when you want to start a local P2P node, connect it to another peer, test peer discovery, use a bootstrap registry, or inspect peer connection behavior.

```bash
vix p2p --id <node_id> --listen <port> [options]
```

## Overview

`vix p2p` starts a TCP-based P2P node.

It can:

```txt
listen for incoming peer connections
connect to another peer manually
discover peers on the local network
bootstrap peers from an HTTP registry
announce itself to a registry
run for a fixed duration
stream live runtime statistics
dedupe repeated connection attempts
apply backoff after failed connections
handle SIGINT cleanly
```

The command is useful for local testing, demos, debugging, and validating the P2P layer before integrating it into a larger application.

## Usage

```bash
vix p2p --id <node_id> --listen <port> [options]
```

## Required options

| Option            | Description                 |
| ----------------- | --------------------------- |
| `--id <node_id>`  | Unique node identifier.     |
| `--listen <port>` | TCP port used by this node. |

Example:

```bash
vix p2p --id A --listen 9001
```

If either `--id` or `--listen` is missing, Vix stops and prints a clear error.

## Basic usage

Start node A:

```bash
vix p2p --id A --listen 9001
```

Start node B in another terminal and connect it to node A:

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

You can also use a TCP URL-style endpoint:

```bash
vix p2p --id B --listen 9002 --connect tcp://127.0.0.1:9001
```

## What happens

When you run:

```bash
vix p2p --id A --listen 9001
```

Vix:

```txt
creates a P2P node
opens a TCP listener
starts discovery when enabled
starts bootstrap when enabled
starts the heartbeat loop
prints runtime statistics
waits until stopped
```

The node can accept incoming peers and can also connect to peers explicitly with `--connect`.

## Run two local nodes

Terminal 1:

```bash
vix p2p --id A --listen 9001
```

Terminal 2:

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

This is the simplest local test.

Node A listens.

Node B listens and connects to node A.

## Manual connection

Use `--connect` to connect to a peer when the node starts.

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

With a delay:

```bash
vix p2p --id B \
  --listen 9002 \
  --connect 127.0.0.1:9001 \
  --connect-delay 1000
```

Use `--connect-delay` when the peer may need a moment to start.

## Disable auto connection

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001 --no-connect
```

`--no-connect` disables automatic connection behavior.

This is useful when you want the node to listen, discover, or bootstrap without immediately dialing a peer.

## Auto-stop

Use `--run <seconds>` to stop automatically after a fixed duration.

```bash
vix p2p --id A --listen 9001 --run 10
```

This is useful for demos, scripts, tests, and CI-style checks.

Example:

```bash
vix p2p --id A --listen 9001 --run 10 --quiet
```

## Stats output

By default, `vix p2p` prints runtime stats.

The stats include:

```txt
peers_total
peers_connected
handshakes_started
handshakes_completed
connect_attempts
connect_deduped
connect_failures
backoff_skips
tracked_endpoints
```

Example shape:

```txt
[vix p2p] peers_total=1 peers_connected=1 handshakes_started=1 handshakes_completed=1 connect_attempts=1 connect_deduped=0 connect_failures=0 backoff_skips=0 tracked_endpoints=1
```

## Stats interval

Use `--stats-every <ms>` to control how often stats are printed.

```bash
vix p2p --id A --listen 9001 --stats-every 500
```

Default:

```txt
1000 ms
```

## TUI mode

`vix p2p` can print live stats in a single updating terminal line when stdout is a TTY.

Force TUI on:

```bash
vix p2p --id A --listen 9001 --tui on
```

Force TUI off:

```bash
vix p2p --id A --listen 9001 --tui off
```

Use `--tui off` when you want clean line-by-line logs.

## Quiet mode

Use `--quiet` to reduce output.

```bash
vix p2p --id A --listen 9001 --run 10 --quiet
```

This is useful for scripts where you only want minimal output.

## Discovery

Discovery lets nodes find each other on the local network.

Discovery is enabled by default.

```bash
vix p2p --id A --listen 9001 --discovery on
```

Disable discovery:

```bash
vix p2p --id A --listen 9001 --discovery off
```

Set discovery port:

```bash
vix p2p --id A --listen 9001 --disc-port 37020
```

Set discovery mode:

```bash
vix p2p --id A --listen 9001 --disc-mode broadcast
vix p2p --id A --listen 9001 --disc-mode multicast
```

Set discovery interval:

```bash
vix p2p --id A --listen 9001 --disc-interval 2000
```

## Local discovery demo

Terminal 1:

```bash
vix p2p --id A \
  --listen 9001 \
  --discovery on \
  --disc-port 37020
```

Terminal 2:

```bash
vix p2p --id B \
  --listen 9002 \
  --discovery on \
  --disc-port 37020
```

Both nodes must use the same discovery port.

## Bootstrap registry

Bootstrap lets a node pull peers from an HTTP registry.

```bash
vix p2p --id A \
  --listen 9001 \
  --bootstrap on \
  --registry http://127.0.0.1:8080
```

By default, bootstrap is off.

Enable it with:

```bash
--bootstrap on
```

Provide a registry endpoint with:

```bash
--registry <url>
```

Example:

```bash
vix p2p --id A \
  --listen 9001 \
  --bootstrap on \
  --registry http://127.0.0.1:8080 \
  --boot-interval 15
```

## Bootstrap announce mode

The node can announce itself to the registry.

```bash
vix p2p --id A \
  --listen 9001 \
  --bootstrap on \
  --registry http://127.0.0.1:8080 \
  --announce on
```

Disable announce:

```bash
vix p2p --id A \
  --listen 9001 \
  --bootstrap on \
  --registry http://127.0.0.1:8080 \
  --announce off
```

When announce is enabled, the node can publish its node id and TCP port to the registry.

## Expected bootstrap registry shape

The bootstrap registry should expose a peer list.

Expected response shape:

```json
{
  "peers": [
    {
      "node_id": "A",
      "host": "127.0.0.1",
      "tcp_port": 9001
    }
  ]
}
```

The bootstrap client reads peers from this response and attempts to connect to them.

## Bootstrap endpoints

The HTTP bootstrap implementation expects two logical endpoints:

```txt
GET  /peers?limit=<n>
POST /announce
```

`GET /peers` returns peer candidates.

`POST /announce` lets a node announce itself when announce mode is enabled.

## Logging

Use `--log-level` to control logs.

```bash
vix p2p --id A --listen 9001 --log-level debug
```

Supported values:

```txt
trace
debug
info
warn
warning
error
err
critical
fatal
off
none
```

Examples:

```bash
vix p2p --id A --listen 9001 --log-level trace
vix p2p --id A --listen 9001 --log-level info
vix p2p --id A --listen 9001 --log-level off
```

## Connection guard

`vix p2p` protects the node from repeated connection attempts.

It tracks endpoints and applies:

```txt
deduplication
failure counting
backoff
tracked endpoint statistics
```

This matters for discovery and bootstrap because the same peer can be found repeatedly.

Instead of blindly dialing the same endpoint again and again, Vix can skip duplicate attempts or wait until backoff expires.

## Manual vs automatic attempts

Manual attempts come from:

```txt
--connect
```

Automatic attempts can come from:

```txt
discovery
bootstrap
```

Automatic attempts are deduped more aggressively to avoid repeated network noise.

Manual attempts still get backoff after failures.

## Runtime stats

The P2P runtime exposes node stats and connection guard stats.

Node stats include:

```txt
peers_total
peers_connected
handshakes_started
handshakes_completed
```

Connection stats include:

```txt
connect_attempts
connect_deduped
connect_failures
backoff_skips
tracked_endpoints
```

These counters help you see whether the node is connecting, deduping, failing, or backing off.

## Peer lifecycle

A peer can move through several states:

```txt
Disconnected
Connecting
Handshaking
Connected
Stale
Closed
```

The CLI is not only opening a socket.

It starts a real peer lifecycle with handshakes, connection state, heartbeat, and peer tracking.

## Handshake behavior

When peers connect, Vix performs a handshake.

The handshake uses messages such as:

```txt
Hello
HelloAck
HelloFinish
```

After a successful handshake, the peer becomes connected.

The node tracks:

```txt
handshakes_started
handshakes_completed
```

These counters are visible in CLI stats.

## Heartbeat behavior

The node sends heartbeat messages to connected peers.

The heartbeat uses:

```txt
Ping
Pong
```

If a peer becomes stale, the node can close the connection.

This helps detect inactive or dead peers.

## Security behavior

The P2P layer has cryptographic hooks.

The node can attach a crypto provider and stores peer metadata such as:

```txt
public key
secure flag
session key
send nonce counter
receive nonce tracking
anti-replay window
```

This means the P2P model is designed for secure sessions, replay protection, and encrypted message support.

## Core options

| Option                        | Description                          |                                       |
| ----------------------------- | ------------------------------------ | ------------------------------------- |
| `--id <node_id>`              | Unique node identifier.              |                                       |
| `--listen <port>`             | TCP listen port.                     |                                       |
| `--connect <host:port>`       | Connect to a peer on startup.        |                                       |
| `--connect tcp://<host:port>` | Connect using TCP endpoint notation. |                                       |
| `--connect-delay <ms>`        | Wait before connecting.              |                                       |
| `--run <seconds>`             | Auto-stop after N seconds.           |                                       |
| `--stats-every <ms>`          | Stats interval. Default is `1000`.   |                                       |
| `--tui <on                    | off>`                                | Enable or disable live stats display. |
| `--quiet`                     | Reduce output.                       |                                       |
| `--no-connect`                | Disable automatic connect behavior.  |                                       |
| `-h, --help`                  | Show command help.                   |                                       |

## Discovery options

| Option                 | Description                                 |                                               |
| ---------------------- | ------------------------------------------- | --------------------------------------------- |
| `--discovery <on       | off>`                                       | Enable or disable discovery. Default is `on`. |
| `--disc-port <port>`   | Discovery port. Default is `37020`.         |                                               |
| `--disc-mode <mode>`   | Discovery mode: `broadcast` or `multicast`. |                                               |
| `--disc-interval <ms>` | Discovery interval. Default is `2000`.      |                                               |

## Bootstrap options

| Option                  | Description                                 |                                                         |
| ----------------------- | ------------------------------------------- | ------------------------------------------------------- |
| `--bootstrap <on        | off>`                                       | Enable or disable registry bootstrap. Default is `off`. |
| `--registry <url>`      | Bootstrap registry endpoint.                |                                                         |
| `--boot-interval <sec>` | Registry refresh interval. Default is `15`. |                                                         |
| `--announce <on         | off>`                                       | Announce node to registry. Default is `on`.             |

## Logging options

| Option                | Description            |
| --------------------- | ---------------------- |
| `--log-level <level>` | Set runtime log level. |

Supported levels:

```txt
trace
debug
info
warn
error
critical
off
```

Aliases:

```txt
warning -> warn
err     -> error
fatal   -> critical
none    -> off
```

## Common workflows

### Run one node

```bash
vix p2p --id A --listen 9001
```

### Run two local nodes

Terminal 1:

```bash
vix p2p --id A --listen 9001
```

Terminal 2:

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

### Run a short demo

```bash
vix p2p --id A --listen 9001 --run 10
```

### Run quietly in a script

```bash
vix p2p --id A --listen 9001 --run 10 --quiet
```

### Use explicit stats interval

```bash
vix p2p --id A --listen 9001 --stats-every 500
```

### Disable TUI output

```bash
vix p2p --id A --listen 9001 --tui off
```

### Use discovery

Terminal 1:

```bash
vix p2p --id A \
  --listen 9001 \
  --discovery on \
  --disc-port 37020
```

Terminal 2:

```bash
vix p2p --id B \
  --listen 9002 \
  --discovery on \
  --disc-port 37020
```

### Use bootstrap registry

```bash
vix p2p --id A \
  --listen 9001 \
  --bootstrap on \
  --registry http://127.0.0.1:8080 \
  --boot-interval 15 \
  --announce on
```

### Debug connection behavior

```bash
vix p2p --id B \
  --listen 9002 \
  --connect 127.0.0.1:9001 \
  --log-level debug \
  --tui off
```

## Example output

Output shape:

```txt
[vix p2p] peers_total=1 peers_connected=1 handshakes_started=1 handshakes_completed=1 connect_attempts=1 connect_deduped=0 connect_failures=0 backoff_skips=0 tracked_endpoints=1
```

This tells you:

```txt
how many peers are known
how many peers are connected
whether handshakes are happening
whether connection attempts are being deduped
whether backoff is active
```

## Common mistakes

### Missing required options

Wrong:

```bash
vix p2p
```

Correct:

```bash
vix p2p --id A --listen 9001
```

### Reusing the same port

Wrong:

```bash
vix p2p --id A --listen 9001
vix p2p --id B --listen 9001
```

Correct:

```bash
vix p2p --id A --listen 9001
vix p2p --id B --listen 9002
```

Each local node needs its own TCP listen port.

### Reusing the same node id

Wrong:

```bash
vix p2p --id A --listen 9001
vix p2p --id A --listen 9002
```

Correct:

```bash
vix p2p --id A --listen 9001
vix p2p --id B --listen 9002
```

Use one unique id per node.

### Connecting before the peer is ready

Start the listening peer first.

Or use:

```bash
vix p2p --id B \
  --listen 9002 \
  --connect 127.0.0.1:9001 \
  --connect-delay 1000
```

### Using different discovery ports

Wrong:

```bash
vix p2p --id A --listen 9001 --discovery on --disc-port 37020
vix p2p --id B --listen 9002 --discovery on --disc-port 37021
```

Correct:

```bash
vix p2p --id A --listen 9001 --discovery on --disc-port 37020
vix p2p --id B --listen 9002 --discovery on --disc-port 37020
```

Discovery peers must use the same discovery port.

### Enabling bootstrap without a registry URL

Wrong:

```bash
vix p2p --id A --listen 9001 --bootstrap on
```

Correct:

```bash
vix p2p --id A \
  --listen 9001 \
  --bootstrap on \
  --registry http://127.0.0.1:8080
```

### Expecting discovery to work across every network

Local discovery depends on network behavior.

Firewalls, containers, VPNs, Wi-Fi isolation, and cloud networks can block broadcast or multicast traffic.

For non-local networks, use bootstrap.

### Expecting `--quiet` to help debugging

`--quiet` reduces output.

For debugging, prefer:

```bash
vix p2p --id A --listen 9001 --log-level debug --tui off
```

## Troubleshooting

### Port already in use

Use another port:

```bash
vix p2p --id A --listen 9003
```

Or find the process using the port:

```bash
sudo lsof -i :9001
```

### Node does not connect

Check that the target node is already running:

```bash
vix p2p --id A --listen 9001
```

Then connect:

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

If startup timing is the issue:

```bash
vix p2p --id B \
  --listen 9002 \
  --connect 127.0.0.1:9001 \
  --connect-delay 1000
```

### Discovery does not find peers

Check:

```txt
both nodes use --discovery on
both nodes use the same --disc-port
firewall allows local discovery traffic
nodes are on the same network
broadcast or multicast is supported
```

Try explicit connection first:

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

### Bootstrap does not return peers

Check the registry URL:

```bash
curl http://127.0.0.1:8080/peers?limit=20
```

Expected shape:

```json
{
  "peers": []
}
```

If announce is enabled, check that the registry supports:

```txt
POST /announce
```

### Too many connection attempts

Look at:

```txt
connect_deduped
backoff_skips
tracked_endpoints
```

If `connect_deduped` increases, Vix is protecting the node from repeated automatic attempts.

If `backoff_skips` increases, endpoints are failing and Vix is waiting before retrying.

### Handshakes start but do not complete

Look at:

```txt
handshakes_started
handshakes_completed
```

If started increases but completed does not, the peer may be disconnecting, handshake validation may be failing, or the remote node may not be compatible.

Use:

```bash
vix p2p --id A --listen 9001 --log-level debug --tui off
```

## Best practices

Use unique node ids.

Use unique listen ports for local nodes.

Start the first node before connecting the second node.

Use `--connect-delay` in scripts.

Use `--run` for demos and automated checks.

Use `--tui off` when saving logs.

Use `--log-level debug` when diagnosing peer behavior.

Use discovery for local network demos.

Use bootstrap for non-local or structured peer discovery.

Use the stats counters to understand what the node is doing.

## Related commands

| Command      | Purpose                                                   |
| ------------ | --------------------------------------------------------- |
| `vix run`    | Run a C++ file, project, script, or app.                  |
| `vix dev`    | Run a project in development mode.                        |
| `vix check`  | Validate project health.                                  |
| `vix info`   | Inspect Vix paths and local state.                        |
| `vix doctor` | Check the local environment.                              |
| `vix logs`   | Inspect production logs when used with deployed services. |
| `vix health` | Check local, public, or WebSocket service health.         |

## Next step

Continue with environment information.

[Open the vix info guide](/cli/info)
