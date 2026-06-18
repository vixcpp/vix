# vix health

`vix health` checks whether a Vix application is reachable and healthy.

Use it when you want to verify local endpoints, public HTTPS endpoints, or WebSocket endpoints in production.

```bash
vix health
```

## Overview

`vix health` is the production health-check command for Vix.

It can check:

- local application endpoint
- public HTTPS endpoint
- WebSocket endpoint
- expected HTTP status
- response time
- configured systemd service state
- production health config from `vix.json`

It is useful after:

- starting a service
- restarting a service
- deploying a new version
- changing Nginx proxy config
- enabling TLS
- debugging production availability

## Usage

```bash
vix health [command]
```

## Commands

| Command                | Purpose                                |
| ---------------------- | -------------------------------------- |
| `vix health`           | Check all configured health endpoints. |
| `vix health local`     | Check the local application endpoint.  |
| `vix health public`    | Check the public endpoint.             |
| `vix health websocket` | Check the WebSocket endpoint.          |
| `vix health ws`        | Alias for `websocket`.                 |

## Basic examples

```bash
# Check all configured endpoints
vix health

# Check local app health
vix health local

# Check public HTTPS health
vix health public

# Check WebSocket health
vix health websocket

# Same as websocket
vix health ws
```

## Configuration source

`vix health` reads health configuration from:

```txt
vix.json
```

under:

```txt
production.health
```

Example:

```json
{
  "name": "PulseGrid",
  "production": {
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/",
      "public": "https://pulsegrid.example.com/",
      "websocket": "wss://pulsegrid.example.com/ws",
      "expected_status": 200,
      "timeout_ms": 3000,
      "max_response_ms": 1000
    }
  }
}
```

## Health config fields

| Field                               | Purpose                                                                |
| ----------------------------------- | ---------------------------------------------------------------------- |
| `production.health.service`         | Optional systemd service name to check before local/all health checks. |
| `production.health.local`           | Local HTTP endpoint.                                                   |
| `production.health.public`          | Public HTTP or HTTPS endpoint.                                         |
| `production.health.websocket`       | WebSocket endpoint.                                                    |
| `production.health.expected_status` | Expected HTTP status for local and public checks. Default: `200`.      |
| `production.health.timeout_ms`      | Request timeout in milliseconds.                                       |
| `production.health.max_response_ms` | Maximum allowed response time in milliseconds.                         |

## Local health

Local health checks the application directly on the server.

Example config:

```json
{
  "production": {
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/"
    }
  }
}
```

Run:

```bash
vix health local
```

Expected output shape:

```txt
Health Check
Target: local
URL: http://127.0.0.1:8080/
Expected: 200
Status: 200
Time: 12 ms
Max time: 1000 ms
Healthy: yes

local endpoint is healthy
```

Use local health to verify that the app itself is alive before debugging Nginx, TLS, DNS, or the public URL.

## Public health

Public health checks the endpoint exposed through your domain.

Example config:

```json
{
  "production": {
    "health": {
      "public": "https://pulsegrid.example.com/"
    }
  }
}
```

Run:

```bash
vix health public
```

Expected output shape:

```txt
Health Check
Target: public
URL: https://pulsegrid.example.com/
Expected: 200
Status: 200
Time: 80 ms
Max time: 1000 ms
Healthy: yes

public endpoint is healthy
```

Use public health after configuring:

```bash
vix proxy nginx init
vix proxy nginx certbot
vix proxy nginx check
```

## WebSocket health

WebSocket health checks the configured WebSocket endpoint.

Example config:

```json
{
  "production": {
    "health": {
      "websocket": "wss://pulsegrid.example.com/ws"
    }
  }
}
```

Run:

```bash
vix health websocket
```

or:

```bash
vix health ws
```

The WebSocket check sends a WebSocket upgrade request.

It expects:

```txt
101
```

because a successful WebSocket handshake normally returns HTTP `101 Switching Protocols`.

## WebSocket URL conversion

For the underlying request, Vix converts WebSocket URLs to HTTP-style URLs:

```txt
ws://  -> http://
wss:// -> https://
```

Examples:

```txt
ws://127.0.0.1:9090/ws
  -> http://127.0.0.1:9090/ws

wss://pulsegrid.example.com/ws
  -> https://pulsegrid.example.com/ws
```

Then Vix sends upgrade headers such as:

```txt
Connection: Upgrade
Upgrade: websocket
Sec-WebSocket-Key: ...
Sec-WebSocket-Version: 13
```

## Check all endpoints

Run:

```bash
vix health
```

This prints the configured health summary, then checks every enabled endpoint.

Example config summary:

```txt
Health Config
App: PulseGrid
Service: pulsegrid.service
Local: http://127.0.0.1:8080/
Public: https://pulsegrid.example.com/
WebSocket: wss://pulsegrid.example.com/ws
```

If no health endpoint is configured, Vix fails with a fix:

```txt
no health endpoints configured
Fix: add production.health to vix.json
```

## Service check

If `production.health.service` is configured, Vix checks whether the service is active before local or all checks.

Example:

```json
{
  "production": {
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/"
    }
  }
}
```

If the service is not running:

```txt
configured service is not running
Fix: run `vix service status`
```

This avoids misleading health checks when the app process is not active.

## Expected status

For local and public health checks, the default expected status is:

```txt
200
```

You can override it:

```json
{
  "production": {
    "health": {
      "local": "http://127.0.0.1:8080/health",
      "expected_status": 204
    }
  }
}
```

For WebSocket checks, the expected status is:

```txt
101
```

The WebSocket expected status is fixed by the WebSocket check behavior.

## Timeout

Use `timeout_ms` to control request timeout.

Example:

```json
{
  "production": {
    "health": {
      "local": "http://127.0.0.1:8080/",
      "timeout_ms": 3000
    }
  }
}
```

If the endpoint does not respond before the timeout, the check fails.

## Maximum response time

Use `max_response_ms` to enforce a performance threshold.

Example:

```json
{
  "production": {
    "health": {
      "public": "https://pulsegrid.example.com/",
      "max_response_ms": 1000
    }
  }
}
```

If the response takes longer than the configured max time, Vix reports:

```txt
response time exceeded
```

## Health result output

Every health check prints:

| Field      | Meaning                               |
| ---------- | ------------------------------------- |
| `Target`   | `local`, `public`, or `websocket`.    |
| `URL`      | Endpoint being checked.               |
| `Expected` | Expected status code.                 |
| `Status`   | Actual status or `connection failed`. |
| `Time`     | Response time in milliseconds.        |
| `Max time` | Maximum allowed response time.        |
| `Healthy`  | `yes` or `no`.                        |
| `Error`    | Error message when unhealthy.         |

## Exit codes

`vix health` returns:

| Exit code | Meaning                                                                         |
| --------- | ------------------------------------------------------------------------------- |
| `0`       | Health check passed.                                                            |
| `1`       | Health check failed, endpoint missing, service not running, or unknown command. |

This makes it useful in scripts and deployment pipelines.

## Full production health example

```json
{
  "name": "PulseGrid",
  "production": {
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/",
      "public": "https://pulsegrid.example.com/",
      "websocket": "wss://pulsegrid.example.com/ws",
      "expected_status": 200,
      "timeout_ms": 3000,
      "max_response_ms": 1000
    }
  }
}
```

Then run:

```bash
vix health
```

or check each endpoint separately:

```bash
vix health local
vix health public
vix health websocket
```

## Relationship with `vix service`

`vix service` manages the application process.

`vix health` checks whether the application responds.

A normal production check is:

```bash
vix service status
vix health local
```

After restart:

```bash
vix service restart
vix health local
```

## Relationship with `vix proxy nginx`

`vix proxy nginx` manages Nginx.

`vix health public` checks the public endpoint after Nginx and TLS are configured.

A normal proxy check is:

```bash
vix proxy nginx check
vix health public
```

For WebSocket apps:

```bash
vix proxy nginx check
vix health websocket
```

## Relationship with `vix doctor production`

`vix doctor production` inspects the full production state.

`vix health` focuses only on endpoint health.

Use both:

```bash
vix health
vix doctor production
```

`vix doctor production` can use health readiness as part of a broader production readiness picture.

## Relationship with `vix deploy`

A deployment flow can run health checks after restarting the service.

Example:

```bash
vix deploy
vix health
vix doctor production
```

If health fails, inspect:

```bash
vix service status
vix service logs
vix proxy nginx check
vix logs
```

## Recommended production workflow

After installing the service:

```bash
vix service start
vix health local
```

After setting up the proxy:

```bash
vix proxy nginx check
vix health public
```

After setting up WebSocket proxying:

```bash
vix proxy nginx check
vix health websocket
```

After deployment:

```bash
vix service restart
vix health
vix doctor production
```

## Commands reference

| Command                | Description                     |
| ---------------------- | ------------------------------- |
| `vix health`           | Check all configured endpoints. |
| `vix health local`     | Check the local endpoint.       |
| `vix health public`    | Check the public endpoint.      |
| `vix health websocket` | Check the WebSocket endpoint.   |
| `vix health ws`        | Alias for WebSocket health.     |
| `vix health --help`    | Show help.                      |

## Common workflows

### Check local app only

```bash
vix health local
```

### Check public app only

```bash
vix health public
```

### Check WebSocket endpoint

```bash
vix health ws
```

### Check everything configured

```bash
vix health
```

### Check after service restart

```bash
vix service restart
vix health local
```

### Check after proxy setup

```bash
vix proxy nginx check
vix health public
```

### Check after TLS setup

```bash
vix proxy nginx certbot
vix proxy nginx check
vix health public
```

### Check after deployment

```bash
vix deploy
vix health
vix doctor production
```

## Common mistakes

### Running health without configuration

Wrong:

```bash
vix health
```

without `production.health`.

Correct:

```json
{
  "production": {
    "health": {
      "local": "http://127.0.0.1:8080/"
    }
  }
}
```

Then run:

```bash
vix health local
```

### Expecting local health to test Nginx

Local health checks the app directly.

For Nginx/public URL, use:

```bash
vix health public
```

### Expecting public health to debug the app process

Public health goes through DNS, TLS, Nginx, and the app.

If public health fails, check the lower layers:

```bash
vix service status
vix health local
vix proxy nginx check
```

### Expecting WebSocket health to return 200

WebSocket health expects:

```txt
101
```

because a successful WebSocket handshake switches protocols.

### Forgetting the service check

If you configure:

```json
{
  "production": {
    "health": {
      "service": "pulsegrid.service"
    }
  }
}
```

and the service is stopped, Vix fails before the local/all health check.

Start or inspect the service:

```bash
vix service status
vix service start
```

### Setting `max_response_ms` too low

If your endpoint is healthy but slower than the threshold, Vix reports:

```txt
response time exceeded
```

Increase the threshold or optimize the endpoint.

## Troubleshooting

### No health endpoints configured

Add `production.health` to `vix.json`.

Example:

```json
{
  "production": {
    "health": {
      "local": "http://127.0.0.1:8080/"
    }
  }
}
```

### Configured service is not running

Run:

```bash
vix service status
```

Start or restart:

```bash
vix service start
vix service restart
```

### Local health fails

Check that the app is running:

```bash
vix service status
```

Check logs:

```bash
vix service logs
```

Check that the app listens on the configured port:

```bash
ss -tulpn
```

### Public health fails

Check Nginx:

```bash
vix proxy nginx check
```

Check local health:

```bash
vix health local
```

Check service logs:

```bash
vix service logs
```

Common causes:

- service stopped
- wrong public domain
- DNS not pointing to the server
- Nginx config invalid
- wrong upstream port
- TLS certificate missing or invalid
- firewall blocks ports `80` or `443`

### WebSocket health fails

Check proxy configuration:

```bash
vix proxy nginx check
```

Check that the app listens on the configured WebSocket port.

Check the WebSocket health URL:

```json
{
  "production": {
    "health": {
      "websocket": "wss://pulsegrid.example.com/ws"
    }
  }
}
```

Common causes:

- wrong WebSocket path
- wrong WebSocket port
- missing Nginx upgrade headers
- app WebSocket server not running
- TLS or proxy issue

### Unexpected HTTP status

If Vix reports:

```txt
unexpected HTTP status
```

the endpoint responded, but not with the expected status.

Check the route and expected status.

Example:

```json
{
  "production": {
    "health": {
      "local": "http://127.0.0.1:8080/health",
      "expected_status": 204
    }
  }
}
```

### Connection failed

If status is:

```txt
connection failed
```

the endpoint could not be reached.

Check:

```bash
vix service status
vix service logs
vix proxy nginx check
```

## Best practices

Keep health configuration in `vix.json`.

Always define a local health endpoint for backend apps.

Define a public health endpoint when the app is exposed through a domain.

Define WebSocket health only when the app exposes WebSocket.

Use `/health` or `/` as a lightweight endpoint.

Keep health endpoints fast.

Use `max_response_ms` to catch slow deployments.

Run `vix health` after every deployment.

Run `vix doctor production` after health checks.

Use `vix service logs` or `vix logs` when health fails.

## Related commands

| Command                 | Purpose                                   |
| ----------------------- | ----------------------------------------- |
| `vix service`           | Manage the systemd app service.           |
| `vix proxy nginx`       | Manage Nginx reverse proxy configuration. |
| `vix doctor production` | Inspect complete production readiness.    |
| `vix logs`              | Inspect production logs.                  |
| `vix deploy`            | Deploy a production app.                  |
| `vix build`             | Build the app binary.                     |
| `vix run`               | Run the app manually.                     |

## Next step

Inspect production readiness.

[Open the production doctor guide](/cli/doctor)
