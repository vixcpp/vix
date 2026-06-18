# Production: Nginx + systemd

This guide shows a practical production workflow for running a Vix application on a Linux server with systemd and Nginx.

Use it when you want to run your app as a service, expose it behind a domain, enable HTTPS, check health, inspect logs, and deploy safely.

## The goal

A typical production setup looks like this:

```txt
public internet
  -> Nginx
  -> Vix app on 127.0.0.1:<port>
  -> systemd keeps the app running
```

For WebSocket apps:

```txt
public internet
  -> Nginx WebSocket location
  -> Vix WebSocket server on 127.0.0.1:<ws_port>
```

Vix gives you production commands for this workflow:

```bash
vix env check --production
vix service init
vix proxy nginx init
vix health
vix logs
vix deploy
```

## Before you start

This guide assumes:

```txt
Linux server
Git installed
Nginx installed
systemd available
Vix CLI installed
your app already builds with vix build
your app can run with vix run
```

Check the machine first:

```bash
vix doctor
vix info
```

## Recommended production flow

Use this order:

```bash
vix env check --production
vix build --preset release
vix check --tests
vix service init
vix proxy nginx init
vix health
vix logs
```

After the first setup, deployments can use:

```bash
vix deploy
```

## Production configuration in `vix.json`

A production app should describe service, proxy, health, logs, and deploy behavior in `vix.json`.

Example shape:

```json
{
  "production": {
    "service": {
      "name": "myapp",
      "user": "www-data",
      "working_dir": "/var/www/myapp",
      "command": "vix run",
      "env_file": "/var/www/myapp/.env"
    },
    "proxy": {
      "domain": "api.example.com",
      "http_port": 8080,
      "websocket": {
        "enabled": true,
        "path": "/ws",
        "port": 9090
      },
      "tls": {
        "enabled": true,
        "certificate": "/etc/letsencrypt/live/api.example.com/fullchain.pem",
        "certificate_key": "/etc/letsencrypt/live/api.example.com/privkey.pem"
      }
    },
    "health": {
      "service": "myapp",
      "local": "http://127.0.0.1:8080/health",
      "public": "https://api.example.com/health",
      "websocket": "wss://api.example.com/ws"
    },
    "logs": {
      "service": "myapp",
      "nginx_access": "/var/log/nginx/api.example.com.access.log",
      "nginx_error": "/var/log/nginx/api.example.com.error.log"
    },
    "deploy": {
      "pull": true,
      "branch": "main",
      "build": "vix build --preset release",
      "tests": true,
      "test_command": "vix check --tests",
      "service": "myapp",
      "health_local": true,
      "health_public": true,
      "proxy_check": true,
      "proxy_reload": true,
      "logs_on_failure": true,
      "log_lines": 100,
      "rollback": true
    }
  }
}
```

Adjust names, paths, ports, and domain for your server.

## Environment files

Use `.env` for runtime configuration:

```dotenv
SERVER_PORT=8080
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
APP_ENV=production
```

For production validation, define required variables in your production config or required env file.

Check local env:

```bash
vix env check
```

Check production env:

```bash
vix env check --production
```

Show values safely:

```bash
vix env check --production --show-values
```

Secrets stay masked.

## Build before production

Before creating services or deploying, make sure the project builds:

```bash
vix build --preset release
```

Then validate:

```bash
vix check --tests
```

This avoids creating a systemd service for an app that cannot build.

## Create the systemd service

Use:

```bash
vix service init
```

This generates, installs, and enables a systemd service from your production config.

Then check it:

```bash
vix service status
```

Restart it:

```bash
vix service restart
```

Stop it:

```bash
vix service stop
```

Start it:

```bash
vix service start
```

The service keeps your Vix app running after SSH disconnects and after server reboots.

## Install the Nginx proxy

Use:

```bash
vix proxy nginx init
```

This generates and installs an Nginx site config.

It can configure:

```txt
HTTP reverse proxy
HTTPS redirect
TLS certificate paths
WebSocket upgrade location
proxy headers
timeouts
Nginx reload
```

Check the proxy:

```bash
vix proxy nginx check
```

Reload Nginx safely:

```bash
vix proxy nginx reload
```

Issue or renew certificates:

```bash
vix proxy nginx certbot
```

## Plain HTTP setup

For early testing, TLS can be disabled.

The proxy forwards:

```txt
http://your-domain/
  -> http://127.0.0.1:8080/
```

Useful for first server validation.

## HTTPS setup

For real production, enable TLS.

The proxy should redirect HTTP to HTTPS:

```txt
http://api.example.com
  -> https://api.example.com
```

Then HTTPS forwards to the local app:

```txt
https://api.example.com
  -> http://127.0.0.1:8080
```

## WebSocket setup

If WebSocket is enabled, Nginx must use upgrade headers.

Vix proxy handles this from config.

Example public WebSocket path:

```txt
wss://api.example.com/ws
```

Local upstream:

```txt
ws://127.0.0.1:9090/ws
```

Check it:

```bash
vix ws check
vix health websocket
```

Use verbose mode when debugging:

```bash
vix ws check --verbose
```

## Health checks

Use:

```bash
vix health
```

This can check:

```txt
local application endpoint
public HTTPS endpoint
WebSocket endpoint
```

Check only local:

```bash
vix health local
```

Check only public:

```bash
vix health public
```

Check WebSocket:

```bash
vix health websocket
```

Recommended after every deployment:

```bash
vix health
```

## Logs

Read production logs with:

```bash
vix logs
```

App logs:

```bash
vix logs app
```

Proxy logs:

```bash
vix logs proxy
```

Errors only:

```bash
vix logs errors
```

Follow logs:

```bash
vix logs app -f
```

Show recent errors:

```bash
vix logs errors --lines 100
```

## Deployment

After service, proxy, health, and logs are configured, use:

```bash
vix deploy
```

A production deploy can:

```txt
pull latest code
build the project
run tests
restart systemd service
check local health
check public health
check proxy config
reload proxy
print logs on failure
rollback when enabled
```

Preview without executing:

```bash
vix deploy --dry-run
```

Verbose mode:

```bash
vix deploy --verbose
```

Skip git pull:

```bash
vix deploy --no-pull
```

Skip tests:

```bash
vix deploy --no-tests
```

## Recommended deployment flow

Use this before a serious deployment:

```bash
vix env check --production
vix deploy --dry-run
vix deploy --verbose
vix health
```

If something fails:

```bash
vix logs errors --lines 100
vix service status
vix proxy nginx check
```

## First-time server setup

A practical first setup:

```bash
git clone https://github.com/example/myapp.git
cd myapp

vix doctor
vix install
vix build --preset release
vix check --tests

cp .env.example .env
vix env check --production

vix service init
vix service status

vix proxy nginx init
vix proxy nginx check

vix health
```

## Normal deployment workflow

After the first setup:

```bash
cd /var/www/myapp
vix deploy
vix health
```

If you want to inspect before running:

```bash
vix deploy --dry-run
```

## Debugging production

### Local health works, public health fails

Run:

```bash
vix health local
vix health public
vix proxy nginx check
vix logs proxy --lines 100
```

This usually means the app is running, but proxy, TLS, DNS, firewall, or public routing needs attention.

### Public HTTP works, WebSocket fails

Run:

```bash
vix health websocket
vix ws check --verbose
vix proxy nginx check
vix logs proxy --lines 100
```

Check the WebSocket path and port.

### Service does not start

Run:

```bash
vix service status
vix logs app --lines 100
vix env check --production
```

Common causes:

```txt
missing env variables
wrong working directory
wrong command
missing executable
port already in use
permission issue
```

### Deployment failed

Run:

```bash
vix logs errors --lines 100
vix service status
vix health
```

Then inspect the step that failed in the deploy output.

## Production safety rules

Do not deploy blindly.

Use:

```bash
vix deploy --dry-run
```

before changing a server for the first time.

Do not delete global state before inspecting it.

Use:

```bash
vix info
```

before cleanup.

Do not hardcode production ports in source code.

Prefer:

```cpp
app.run();
```

and configure the port in `.env`:

```dotenv
SERVER_PORT=8080
```

## Common mistakes

### Running production commands before building

Correct flow:

```bash
vix build --preset release
vix check --tests
vix service init
```

### Forgetting production env validation

Run:

```bash
vix env check --production
```

before deployment.

### Checking only public health

Always check local first:

```bash
vix health local
vix health public
```

If local fails, fix the app or service first.

If local passes and public fails, check Nginx, TLS, DNS, or firewall.

### Forgetting WebSocket proxy config

If your app uses WebSocket, make sure it is enabled in production proxy config and test it:

```bash
vix ws check --verbose
```

### Using `vix clean` for production cache problems

`vix clean` affects project-local state.

For global package store issues, inspect first:

```bash
vix info
vix store path
vix store gc --dry-run
```

## Recommended command map

| Need                         | Command                      |
| ---------------------------- | ---------------------------- |
| Check machine setup          | `vix doctor`                 |
| Inspect Vix paths and caches | `vix info`                   |
| Check production env         | `vix env check --production` |
| Build release                | `vix build --preset release` |
| Validate project             | `vix check --tests`          |
| Create systemd service       | `vix service init`           |
| Check service                | `vix service status`         |
| Create Nginx proxy           | `vix proxy nginx init`       |
| Check Nginx proxy            | `vix proxy nginx check`      |
| Reload Nginx                 | `vix proxy nginx reload`     |
| Check health                 | `vix health`                 |
| Check WebSocket              | `vix ws check`               |
| Read logs                    | `vix logs`                   |
| Deploy                       | `vix deploy`                 |

## Related commands

| Command       | Purpose                                         |
| ------------- | ----------------------------------------------- |
| `vix service` | Manage systemd service                          |
| `vix proxy`   | Manage Nginx reverse proxy                      |
| `vix health`  | Check local, public, and WebSocket endpoints    |
| `vix logs`    | Read app, proxy, and error logs                 |
| `vix env`     | Validate local and production environment files |
| `vix deploy`  | Run production deployment workflow              |
| `vix ws`      | Check WebSocket endpoint                        |
| `vix doctor`  | Diagnose server environment                     |
| `vix info`    | Inspect Vix local state                         |

## What you should remember

A production Vix app should be observable and easy to recover.

The stable workflow is:

```bash
vix env check --production
vix build --preset release
vix check --tests
vix service init
vix proxy nginx init
vix health
```

Then deploy with:

```bash
vix deploy
```

And debug with:

```bash
vix health
vix logs errors --lines 100
vix service status
vix proxy nginx check
```

## Next step

Return to the guides overview.

[Open all guides](/cli/deploy)
