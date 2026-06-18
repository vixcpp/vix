# vix logs

`vix logs` shows production logs for a Vix application.

Use it when you want to inspect systemd app logs, Nginx proxy logs, error logs, repeated failures, or live production output.

```bash
vix logs
```

## Overview

`vix logs` is the production log inspection command for Vix.

It can show:

- systemd app logs
- Nginx access logs
- Nginx error logs
- app errors
- proxy errors
- repeated error groups
- common network disconnect groups
- live logs
- logs since a specific time
- the last N lines
- JSON output for repeated error analysis

It is useful when:

- the app fails to start
- health checks fail
- public traffic does not work
- Nginx proxy behaves incorrectly
- WebSocket clients disconnect
- production logs are too noisy
- deployment failed and you need recent errors

## Platform support

`vix logs` is currently supported on:

```txt
Linux
```

It uses Linux production tools such as:

```txt
journalctl
tail
grep
sudo
```

On unsupported platforms, Vix reports that `vix logs` is currently supported on Linux only.

## Usage

```bash
vix logs [target] [options]
```

## Targets

| Target    | Purpose                               |
| --------- | ------------------------------------- |
| no target | Show app logs and proxy logs.         |
| `app`     | Show systemd app logs.                |
| `proxy`   | Show Nginx access and error logs.     |
| `errors`  | Show app errors and Nginx error logs. |

## Basic examples

```bash
# Show app and proxy logs
vix logs

# Show only app logs
vix logs app

# Show only proxy logs
vix logs proxy

# Show app errors and proxy error logs
vix logs errors

# Follow logs live
vix logs --follow

# Alias for --follow
vix logs -f

# Show only common error lines
vix logs --errors

# Analyze repeated errors
vix logs errors --repeated

# Analyze repeated errors as JSON
vix logs errors --repeated --json

# Show logs since a systemd time expression
vix logs --since "1 hour ago"

# Show last 200 lines
vix logs -n 200

# Same as -n
vix logs --lines 200
```

## Configuration source

`vix logs` reads configuration from:

```txt
vix.json
```

under:

```txt
production.logs
```

It can also reuse service names from:

```txt
production.deploy.service
production.service.name
```

## Full config example

```json
{
  "name": "PulseGrid",
  "production": {
    "service": {
      "name": "pulsegrid"
    },
    "deploy": {
      "service": "pulsegrid"
    },
    "logs": {
      "service": "pulsegrid",
      "nginx_access": "/var/log/nginx/pulsegrid.access.log",
      "nginx_error": "/var/log/nginx/pulsegrid.error.log",
      "lines": 120
    }
  }
}
```

## Logs config fields

| Field                          | Purpose                                 |
| ------------------------------ | --------------------------------------- |
| `production.logs.service`      | systemd service name used for app logs. |
| `production.logs.nginx_access` | Nginx access log path.                  |
| `production.logs.nginx_error`  | Nginx error log path.                   |
| `production.logs.lines`        | Default number of lines to show.        |

## Service name detection

Vix resolves the service name in this order:

```txt
production.logs.service
production.deploy.service
production.service.name
project name
```

If the service name ends with:

```txt
.service
```

Vix normalizes it internally and uses the service name for `journalctl`.

Example:

```json
{
  "production": {
    "logs": {
      "service": "pulsegrid.service"
    }
  }
}
```

is treated as:

```txt
pulsegrid
```

## Default log paths

If no custom Nginx log paths are configured, Vix uses default paths based on the app name.

For an app named:

```txt
PulseGrid
```

the default paths are:

```txt
/var/log/nginx/PulseGrid.access.log
/var/log/nginx/PulseGrid.error.log
```

You can override them:

```json
{
  "production": {
    "logs": {
      "nginx_access": "/var/log/nginx/pulsegrid.access.log",
      "nginx_error": "/var/log/nginx/pulsegrid.error.log"
    }
  }
}
```

## Show all logs

Run:

```bash
vix logs
```

This shows:

```txt
app logs
proxy access logs
proxy error logs
```

It prints a summary first:

```txt
Logs
App: PulseGrid
Target: all
Service: pulsegrid
Nginx access: /var/log/nginx/pulsegrid.access.log
Nginx error: /var/log/nginx/pulsegrid.error.log
Lines: 120
Follow: no
Errors only: no
```

Then it runs the needed system commands.

## Show app logs

Run:

```bash
vix logs app
```

This uses:

```bash
journalctl -u <service> -n <lines> --no-pager
```

Example:

```bash
journalctl -u pulsegrid -n 120 --no-pager
```

Use this when the app:

- fails to start
- exits unexpectedly
- crashes
- logs runtime errors
- cannot bind a port
- cannot open a file
- cannot connect to a database

## Show proxy logs

Run:

```bash
vix logs proxy
```

This shows:

```txt
Nginx access log
Nginx error log
```

It uses commands shaped like:

```bash
sudo tail -n 120 /var/log/nginx/pulsegrid.access.log
sudo tail -n 120 /var/log/nginx/pulsegrid.error.log
```

Use this when:

- public traffic does not reach the app
- Nginx returns 502 or 504
- TLS works but upstream fails
- WebSocket proxying fails
- proxy headers or upstream ports are wrong

## Show error logs

Run:

```bash
vix logs errors
```

This shows:

```txt
filtered app errors
Nginx error log
```

It filters app logs using common error keywords such as:

```txt
error
failed
failure
exception
panic
fatal
critical
timeout
refused
denied
```

Use this when you want to focus on failures instead of normal traffic.

## Error filtering

You can filter logs by common error keywords with:

```bash
vix logs --errors
```

This applies to app logs and proxy logs where supported.

Example:

```bash
vix logs app --errors
vix logs proxy --errors
vix logs --errors
```

The filter looks for common failure words such as:

```txt
error
failed
failure
exception
panic
fatal
critical
timeout
refused
denied
```

## Follow logs live

Use `--follow` or `-f`:

```bash
vix logs --follow
vix logs -f
vix logs app -f
vix logs proxy -f
```

For app logs, this maps to:

```bash
journalctl -u <service> -f
```

For proxy logs, this maps to:

```bash
sudo tail -f <log-file>
```

Use follow mode when you want to watch logs while:

- restarting a service
- testing a route
- hitting the public URL
- connecting a WebSocket client
- running a deployment

## Show logs since a time

Use `--since`:

```bash
vix logs --since "1 hour ago"
vix logs app --since "today"
vix logs errors --since "10 minutes ago"
```

`--since` is passed to `journalctl` for app logs.

Examples of valid systemd time expressions:

```txt
1 hour ago
10 minutes ago
today
yesterday
2026-05-28 10:00:00
```

## Show last N lines

Use `--lines` or `-n`:

```bash
vix logs --lines 200
vix logs -n 200
vix logs app -n 50
vix logs proxy -n 300
```

If not configured, the default is usually:

```txt
120
```

You can also set the default in `vix.json`:

```json
{
  "production": {
    "logs": {
      "lines": 200
    }
  }
}
```

## Repeated error analysis

Use `--repeated` to analyze repeated failures:

```bash
vix logs errors --repeated
```

This reads app and proxy error-like lines, normalizes them, groups repeated messages, and reports common patterns.

Example output shape:

```txt
Repeated Errors
Analyzed lines: 120
Repeated groups: 2
12x upstream timed out
5x connection refused

Common Network Disconnects
Detected groups: 3
Hidden normal noise: 18 lines
14x client disconnected
3x connection reset by peer
1x timeout
```

This is useful when production logs are noisy and you want to know which problems repeat the most.

## Repeated error JSON

Use:

```bash
vix logs errors --repeated --json
```

Example output shape:

```json
{
  "analyzed_lines": 120,
  "repeated_groups": 2,
  "network_disconnect_groups": 3,
  "hidden_normal_noise_lines": 18,
  "repeated_errors": [
    {
      "message": "upstream timed out",
      "count": 12
    }
  ],
  "network_disconnects": [
    {
      "name": "client disconnected",
      "count": 14
    }
  ]
}
```

Use JSON output for:

- CI
- dashboards
- automation
- production monitoring experiments
- deployment reports

## Normal network disconnect noise

Production logs often contain normal disconnects.

Examples:

```txt
broken pipe
connection reset by peer
client closed connection
client prematurely closed connection
websocket disconnected
EOF
```

These do not always mean your app is broken.

They often mean the client closed the connection.

`vix logs errors --repeated` groups this noise separately so it does not hide real repeated failures.

Normal network noise groups include:

```txt
client disconnected
connection reset by peer
websocket disconnected
```

The report can show:

```txt
Hidden normal noise: 18 lines
```

This helps production logs stay useful.

## Network disconnect groups

The repeated error analyzer can group common network failures such as:

| Group                      | Meaning                                                     |
| -------------------------- | ----------------------------------------------------------- |
| `client disconnected`      | Client closed the connection, broken pipe, EOF, or similar. |
| `connection reset by peer` | Remote side reset the connection.                           |
| `websocket disconnected`   | WebSocket client disconnected.                              |
| `upstream disconnected`    | Upstream closed the connection early.                       |
| `connection refused`       | Upstream or target refused the connection.                  |
| `timeout`                  | A request or upstream timed out.                            |

The goal is to separate normal production noise from real repeated failures.

## Commands printed before execution

`vix logs` prints the command it is about to run.

Example:

```txt
Command: journalctl -u pulsegrid -n 120 --no-pager
```

This makes log inspection transparent.

If you need to debug manually, you can copy the command and run it yourself.

## Target behavior

### No target

```bash
vix logs
```

Runs:

```txt
app logs
proxy access logs
proxy error logs
```

### `app`

```bash
vix logs app
```

Runs:

```txt
systemd app logs
```

### `proxy`

```bash
vix logs proxy
```

Runs:

```txt
Nginx access logs
Nginx error logs
```

### `errors`

```bash
vix logs errors
```

Runs:

```txt
filtered app errors
Nginx error logs
```

It also enables error filtering internally.

## Full production logs config

```json
{
  "name": "PulseGrid",
  "production": {
    "service": {
      "name": "pulsegrid"
    },
    "proxy": {
      "domain": "pulsegrid.example.com",
      "http": {
        "port": 8080
      }
    },
    "logs": {
      "service": "pulsegrid",
      "nginx_access": "/var/log/nginx/pulsegrid.access.log",
      "nginx_error": "/var/log/nginx/pulsegrid.error.log",
      "lines": 120
    }
  }
}
```

Then run:

```bash
vix logs
vix logs app
vix logs proxy
vix logs errors
vix logs errors --repeated
```

## Relationship with `vix service`

`vix service` manages the app service.

`vix logs app` reads logs for that service.

Typical flow:

```bash
vix service status
vix logs app
```

After a restart:

```bash
vix service restart
vix logs app -f
```

## Relationship with `vix proxy nginx`

`vix proxy nginx` manages Nginx config.

`vix logs proxy` reads Nginx access and error logs.

Typical flow:

```bash
vix proxy nginx check
vix logs proxy
```

If the public endpoint fails:

```bash
vix proxy nginx check
vix logs proxy --errors
```

## Relationship with `vix health`

`vix health` tells you whether an endpoint is healthy.

`vix logs` tells you why it may be unhealthy.

Typical flow:

```bash
vix health
vix logs errors
vix logs errors --repeated
```

## Relationship with `vix doctor production`

`vix doctor production` gives a readiness overview.

`vix logs` gives detailed runtime evidence.

Typical flow:

```bash
vix doctor production
vix logs errors
```

## Relationship with `vix deploy`

A deployment command can show logs on failure.

Manual flow:

```bash
vix deploy
vix health
vix logs errors
```

If repeated failures appear:

```bash
vix logs errors --repeated
```

## Options

| Option           | Description                                                         |
| ---------------- | ------------------------------------------------------------------- |
| `app`            | Show systemd app logs.                                              |
| `proxy`          | Show Nginx access and error logs.                                   |
| `errors`         | Show app errors and Nginx error logs.                               |
| `--follow`       | Follow logs live.                                                   |
| `-f`             | Alias for `--follow`.                                               |
| `--errors`       | Filter logs by common error keywords.                               |
| `--repeated`     | Detect repeated errors and common network disconnect groups.        |
| `--json`         | Print supported output as JSON. Currently useful with `--repeated`. |
| `--since <time>` | Filter app logs by a systemd time expression.                       |
| `--lines <n>`    | Show last N lines.                                                  |
| `-n <n>`         | Alias for `--lines`.                                                |
| `-h, --help`     | Show help.                                                          |

## Common workflows

### Inspect everything

```bash
vix logs
```

### Inspect app logs

```bash
vix logs app
```

### Inspect proxy logs

```bash
vix logs proxy
```

### Inspect only errors

```bash
vix logs errors
```

### Follow app logs while restarting

```bash
vix service restart
vix logs app -f
```

### Follow proxy logs while testing public URL

```bash
vix logs proxy -f
```

### Show recent errors

```bash
vix logs errors -n 200
```

### Show errors from the last hour

```bash
vix logs errors --since "1 hour ago"
```

### Analyze repeated errors

```bash
vix logs errors --repeated
```

### Analyze repeated errors as JSON

```bash
vix logs errors --repeated --json
```

## Common mistakes

### Running outside the project directory

Wrong:

```bash
cd ~
vix logs
```

Correct:

```bash
cd /path/to/myapp
vix logs
```

`vix logs` reads `vix.json` from the current project directory.

### Expecting logs to start the service

`vix logs` only reads logs.

Start the service with:

```bash
vix service start
```

### Expecting logs to fix health failures

`vix logs` helps you inspect failures.

Use the right command to fix the source of the issue.

Examples:

```bash
vix service restart
vix proxy nginx check
vix health local
vix health public
```

### Using `--json` without `--repeated`

JSON output is currently useful for repeated error analysis.

Use:

```bash
vix logs errors --repeated --json
```

### Expecting `--since` to filter Nginx log files

`--since` is passed to `journalctl` for app logs.

Nginx log files are read with `tail`.

### Forgetting sudo permissions for Nginx logs

Proxy logs are usually under:

```txt
/var/log/nginx/
```

Vix reads them using `sudo tail`.

Make sure your user can use `sudo`.

### Missing Nginx log files

If Vix reports that a log file is missing, check:

```json
{
  "production": {
    "logs": {
      "nginx_access": "/var/log/nginx/pulsegrid.access.log",
      "nginx_error": "/var/log/nginx/pulsegrid.error.log"
    }
  }
}
```

or configure Nginx to write logs at those paths.

## Troubleshooting

### Missing service name

If app logs cannot be read, configure:

```json
{
  "production": {
    "logs": {
      "service": "pulsegrid"
    }
  }
}
```

You can also define:

```json
{
  "production": {
    "service": {
      "name": "pulsegrid"
    }
  }
}
```

### App logs are empty

Check service status:

```bash
vix service status
```

Then inspect directly:

```bash
journalctl -u pulsegrid -n 120 --no-pager
```

### Proxy access log not found

Check the configured path:

```json
{
  "production": {
    "logs": {
      "nginx_access": "/var/log/nginx/pulsegrid.access.log"
    }
  }
}
```

Then check Nginx config:

```bash
vix proxy nginx check
```

### Proxy error log not found

Check:

```json
{
  "production": {
    "logs": {
      "nginx_error": "/var/log/nginx/pulsegrid.error.log"
    }
  }
}
```

Then inspect Nginx:

```bash
sudo nginx -t
```

### Public URL fails

Run:

```bash
vix health public
vix proxy nginx check
vix logs proxy --errors
```

### Local app fails

Run:

```bash
vix health local
vix service status
vix logs app --errors
```

### WebSocket fails

Run:

```bash
vix health websocket
vix proxy nginx check
vix logs errors --repeated
```

### Too many disconnect messages

Run:

```bash
vix logs errors --repeated
```

Normal disconnects are grouped separately so you can focus on actual repeated failures.

## Best practices

Keep log configuration in `vix.json`.

Use `vix logs app` for app crashes and service startup errors.

Use `vix logs proxy` for Nginx and public traffic issues.

Use `vix logs errors` when you want failure-focused logs.

Use `vix logs errors --repeated` when logs are noisy.

Use `vix logs app -f` while restarting the service.

Use `vix logs proxy -f` while testing the public URL.

Use `vix health` before logs to know which layer is failing.

Use `vix doctor production` after logs to confirm readiness.

## Related commands

| Command                 | Purpose                                       |
| ----------------------- | --------------------------------------------- |
| `vix service`           | Manage the systemd app service.               |
| `vix proxy nginx`       | Manage Nginx reverse proxy configuration.     |
| `vix health`            | Check local, public, and WebSocket endpoints. |
| `vix doctor production` | Inspect production readiness.                 |
| `vix deploy`            | Deploy the production app.                    |
| `vix build`             | Build the app binary.                         |
| `vix run`               | Run the app manually.                         |

## Next step

Check endpoint health.

[Open the health guide](/cli/health)
