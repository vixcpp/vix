# vix doctor

`vix doctor` checks your Vix environment.

Use it when you want to diagnose setup problems, verify the local toolchain, inspect production readiness, or check whether Vix can work correctly on the current machine.

```bash
vix doctor
```

## Overview

`vix doctor` is the diagnostic command for Vix.

It can check:

- the installed Vix CLI
- the operating system and architecture
- the local Vix version
- install metadata
- required development tools
- PATH setup
- writable install directories
- online release information
- CLI and CMake package consistency
- production readiness for a deployed app

It is useful during:

- first installation
- environment setup
- upgrade debugging
- project troubleshooting
- CI checks
- production deployment checks

## Usage

```bash
vix doctor [mode] [options]
```

## Basic examples

```bash
# Check local environment
vix doctor

# Check local environment and latest GitHub release
vix doctor --online

# Print JSON summary
vix doctor --json

# Print JSON summary and check latest release online
vix doctor --json --online

# Check a specific GitHub repo when using online mode
vix doctor --online --repo vixcpp/vix

# Check CLI and CMake package consistency
vix doctor toolchain

# Check production readiness
vix doctor production

# Check production readiness with JSON output
vix doctor production --json
```

## Modes

| Mode                    | Purpose                                                                      |
| ----------------------- | ---------------------------------------------------------------------------- |
| `vix doctor`            | Check local Vix environment.                                                 |
| `vix doctor toolchain`  | Check whether the Vix CLI and CMake package come from the same installation. |
| `vix doctor production` | Check production readiness for the current app.                              |

## Local environment doctor

Run:

```bash
vix doctor
```

This checks the local Vix setup.

It can report:

```txt
Doctor
exe: /home/user/.local/bin/vix
os: linux
arch: x86_64
version: v2.6.0
install_stats: /home/user/.local/share/vix/install.json
install_dir: /home/user/.local
```

It can also check common tools needed by Vix workflows.

Examples:

```txt
cmake: ok
ninja: ok
git: ok
curl: ok
```

The exact output depends on the platform and installed tools.

## Online check

Use `--online` to check the latest release from GitHub:

```bash
vix doctor --online
```

By default, Vix checks:

```txt
vixcpp/vix
```

Use another repository:

```bash
vix doctor --online --repo vixcpp/vix
```

Equivalent form:

```bash
vix doctor --online --repo=vixcpp/vix
```

Online mode uses available tools such as:

```txt
curl
wget
PowerShell
```

depending on the platform.

## JSON output

Use `--json` when you want machine-readable output:

```bash
vix doctor --json
```

Use it with online mode:

```bash
vix doctor --json --online
```

Use it with production mode:

```bash
vix doctor production --json
```

JSON output is useful for:

- CI pipelines
- automated diagnostics
- scripts
- dashboards
- production checks

## Toolchain doctor

Run:

```bash
vix doctor toolchain
```

This checks whether the Vix CLI and the CMake package appear to come from the same installation.

This matters because a common confusing situation is:

```txt
vix command comes from one folder
Vix_DIR or CMAKE_PREFIX_PATH points to another folder
```

That can cause builds to use one Vix CLI but link against another Vix SDK.

## What toolchain doctor checks

`vix doctor toolchain` inspects:

- current `vix` executable path
- Vix CLI version
- `Vix_DIR`
- `CMAKE_PREFIX_PATH`
- whether these paths appear to belong to the same installation

Example output:

```txt
Toolchain Consistency

CLI
Path: /home/user/.local/bin/vix
Version: v2.6.0

CMake Package
Vix_DIR: /home/user/.local/lib/cmake/vix
CMAKE_PREFIX_PATH: /home/user/.local

Result
Vix CLI and CMake package appear to come from the same installation
```

If they differ, Vix warns you.

Example:

```txt
Vix CLI and Vix libraries come from different installations
```

This can happen when you have several Vix builds installed locally.

## Fixing toolchain mismatch

If `vix doctor toolchain` reports a mismatch, make sure the CLI and CMake package come from the same installation.

Example:

```bash
export Vix_DIR=/home/user/.local/lib/cmake/vix
export CMAKE_PREFIX_PATH=/home/user/.local
```

Then run:

```bash
vix doctor toolchain
```

again.

## Production doctor

Run:

```bash
vix doctor production
```

This checks whether the current project looks ready for production.

It is designed for real backend deployments where Vix should help you inspect:

- app binary
- build directory
- running process
- systemd service
- restart policy
- working directory
- environment variables
- HTTP port
- WebSocket port
- Nginx reverse proxy
- TLS certificate
- local health
- public health
- public static assets
- readiness score

This command is part of the production direction of Vix.

The goal is to make production C++ apps easier to inspect and operate.

## Production doctor platform support

`vix doctor production` is currently Linux-oriented.

It checks Linux production infrastructure such as:

```txt
systemd
journalctl
nginx
certbot / Let's Encrypt paths
ss
curl
```

On Windows, production doctor is not currently supported.

## What production doctor detects

`vix doctor production` can detect:

| Area           | Checks                                                       |
| -------------- | ------------------------------------------------------------ |
| Project        | project name from `vix.json`, `.vix`, or folder name         |
| Build          | build directory and binary path                              |
| Process        | whether the app binary is running                            |
| Service        | matching systemd service                                     |
| Service status | active state, restart policy, working directory, environment |
| Network        | HTTP listening port                                          |
| WebSocket      | configured WebSocket port                                    |
| Proxy          | Nginx config for the app                                     |
| TLS            | Let's Encrypt certificate presence                           |
| Health         | local and public health checks                               |
| Assets         | expected `public/` files                                     |
| Readiness      | production readiness score                                   |

## Production doctor output

Example shape:

```txt
Production Doctor

App: PulseGrid
Status: running
Build dir: /home/user/PulseGrid/build-ninja
Binary: /home/user/PulseGrid/build-ninja/PulseGrid
Binary exists: yes
Service: pulsegrid.service
Service status: active
Restart policy: always
Working directory: /home/user/PulseGrid
HTTP port: 8080
WebSocket port: 9090
Proxy: nginx
Proxy target: http://127.0.0.1:8080
Public URL: https://pulsegrid.example.com
TLS: enabled
Local health: ok
Public health: ok
Score: 90/100
```

If something is missing, Vix prints a fix.

Example:

```txt
FAIL: service installed (0/15)
Fix: run `vix service install`
```

## Production readiness score

Production doctor gives a readiness score.

The score is based on checks such as:

| Check                       | Points |
| --------------------------- | ------ |
| service installed           | 15     |
| service running             | 15     |
| binary exists               | 10     |
| Nginx proxy configured      | 15     |
| TLS enabled                 | 15     |
| local health check          | 10     |
| public health check         | 10     |
| WebSocket health configured | 5      |
| deploy rollback configured  | 5      |

Example:

```txt
Score: 85/100

OK: service installed (15/15)
OK: service running (15/15)
OK: binary exists (10/10)
OK: nginx proxy configured (15/15)
OK: TLS enabled (15/15)
WARN: websocket health configured (0/5)
WARN: deploy rollback configured (0/5)
```

The score is not magic.

It is a simple way to show whether the app has the basic production pieces in place.

## Public assets check

Production doctor also checks for a `public/` directory.

It can inspect files such as:

```txt
public/index.html
public/app.css
public/app.js
public/status.html
public/status.css
public/status.js
```

If files are missing, Vix warns you.

Example:

```txt
Missing public asset: public/status.html
Fix: create the missing files in public/ or remove static UI expectations from this app.
```

This is useful for backend apps that serve a small production UI, dashboard, or status page.

## Production config in `vix.json`

Production doctor becomes more useful when your project has production configuration in `vix.json`.

Example:

```json
{
  "name": "PulseGrid",
  "production": {
    "service": {
      "name": "pulsegrid",
      "user": "gaspard",
      "working_dir": "/home/gaspard/PulseGrid",
      "exec": "build-ninja/PulseGrid",
      "restart": "always",
      "restart_sec": 3,
      "limit_nofile": 65535
    },
    "ports": {
      "http": 8080,
      "websocket": 9090
    },
    "proxy": {
      "type": "nginx",
      "domain": "pulsegrid.example.com",
      "tls": true,
      "websocket_path": "/ws"
    },
    "health": {
      "local": "http://127.0.0.1:8080/",
      "public": "https://pulsegrid.example.com/",
      "websocket": "wss://pulsegrid.example.com/ws"
    },
    "deploy": {
      "rollback": true
    }
  }
}
```

This config gives Vix one clear place to understand production.

## Health checks

Production doctor can check local and public health.

Local health means the app responds locally:

```txt
http://127.0.0.1:<port>/
```

Public health means the public HTTPS URL responds:

```txt
https://your-domain.com/
```

If health checks fail, Vix suggests what to check next.

Examples:

```txt
Fix: configure production.health.local and run `vix health local`
Fix: configure production.health.public and run `vix health public`
```

## Nginx and TLS checks

Production doctor can detect whether an Nginx config exists for the project.

It can also detect:

- proxy target
- public domain
- TLS certificate presence

For TLS, it checks Let's Encrypt style paths such as:

```txt
/etc/letsencrypt/live/<domain>/fullchain.pem
```

If TLS is missing, Vix suggests enabling it.

Example:

```txt
Fix: run `vix proxy nginx certbot`
```

## Systemd service checks

Production doctor can detect a systemd service linked to the current app.

It checks:

- service name
- active state
- restart policy
- working directory
- environment variables
- whether the service points to the current project or binary

If no service is found, Vix suggests:

```bash
vix service install
```

If the service is not running, Vix suggests:

```bash
vix service start
```

or:

```bash
vix service status
```

## Production JSON output

Use:

```bash
vix doctor production --json
```

Example shape:

```json
{
  "app": "PulseGrid",
  "running": true,
  "build_dir": "/home/user/PulseGrid/build-ninja",
  "binary": "/home/user/PulseGrid/build-ninja/PulseGrid",
  "binary_exists": true,
  "service": "pulsegrid.service",
  "service_exists": true,
  "service_status": "active",
  "restart_policy": "always",
  "working_directory": "/home/user/PulseGrid",
  "http_port": "8080",
  "websocket_port": "9090",
  "proxy": "nginx",
  "proxy_target": "http://127.0.0.1:8080",
  "domain": "pulsegrid.example.com",
  "tls": true,
  "local_health": true,
  "public_health": true,
  "readiness_score": 90,
  "readiness_max": 100
}
```

Use JSON output in CI or production scripts.

## Suggested production workflow

A future production workflow can look like this:

```bash
vix build --preset release
vix service install
vix service start
vix proxy nginx init
vix proxy nginx check
vix health local
vix health public
vix doctor production
```

For deployment:

```bash
vix deploy
vix doctor production
```

This keeps the production workflow inside Vix instead of forcing every project to invent custom scripts.

## Basic troubleshooting flow

When something does not work locally:

```bash
vix doctor
vix info
vix doctor toolchain
vix build -v
```

When package resolution fails:

```bash
vix registry sync
vix install
vix doctor
```

When project state feels broken:

```bash
vix reset
vix doctor
```

When production is broken:

```bash
vix doctor production
vix service status
vix logs
vix health local
vix health public
```

When Vix may be outdated:

```bash
vix doctor --online
vix upgrade --check
```

## Difference between `vix doctor` and `vix info`

| Command      | Purpose                                                      |
| ------------ | ------------------------------------------------------------ |
| `vix doctor` | Checks environment health and readiness.                     |
| `vix info`   | Shows paths, caches, installation state, and local metadata. |

Use `vix doctor` when you want to know whether something is wrong.

Use `vix info` when you want to inspect where Vix stores things.

## Difference between doctor modes

| Command                 | Purpose                                   |
| ----------------------- | ----------------------------------------- |
| `vix doctor`            | Local development environment health.     |
| `vix doctor --online`   | Local health plus latest release check.   |
| `vix doctor toolchain`  | CLI and CMake package consistency.        |
| `vix doctor production` | Production readiness for the current app. |

## Options

| Option                | Description                                                         |
| --------------------- | ------------------------------------------------------------------- |
| `--json`              | Print a JSON summary.                                               |
| `--online`            | Check the latest release on GitHub.                                 |
| `--repo <owner/name>` | Repository to check when using `--online`. Default is `vixcpp/vix`. |
| `--repo=<owner/name>` | Same as `--repo <owner/name>`.                                      |
| `-h, --help`          | Show command help.                                                  |

## Modes

| Mode         | Description                                     |
| ------------ | ----------------------------------------------- |
| `toolchain`  | Check CLI and CMake package consistency.        |
| `production` | Check production readiness for the current app. |

## Common mistakes

### Expecting doctor to fix problems automatically

`vix doctor` diagnoses problems.

It does not automatically fix them.

After diagnosis, use the right command.

Examples:

```bash
vix registry sync
vix install
vix reset
vix upgrade
vix service restart
vix proxy nginx check
```

### Using `--repo` without `--online`

`--repo` only matters when checking GitHub releases.

Use:

```bash
vix doctor --online --repo vixcpp/vix
```

### Running production doctor outside a project

`vix doctor production` expects to inspect the current project.

Run it from the app root:

```bash
cd /path/to/myapp
vix doctor production
```

### Expecting production doctor to create services

`vix doctor production` inspects production state.

It does not create the service.

Use:

```bash
vix service install
```

### Expecting production doctor to create Nginx config

`vix doctor production` checks proxy state.

It does not create the proxy config.

Use:

```bash
vix proxy nginx init
```

### Expecting production doctor to renew TLS

`vix doctor production` checks TLS state.

It does not issue certificates.

Use:

```bash
vix proxy nginx certbot
```

### Ignoring toolchain mismatch warnings

If `vix doctor toolchain` reports that the CLI and CMake package come from different installations, fix it before debugging deeper build issues.

A mismatch can cause confusing builds.

## Troubleshooting

### Vix command is missing

Check:

```bash
command -v vix
```

Make sure the install directory is in `PATH`.

Then run:

```bash
vix doctor
```

### CMake package is not found

Run:

```bash
vix doctor toolchain
```

Check:

```bash
echo $Vix_DIR
echo $CMAKE_PREFIX_PATH
```

Make sure they point to the same Vix installation used by the CLI.

### Production service is not found

Run from the project root:

```bash
vix doctor production
```

If still missing, install the service:

```bash
vix service install
```

Then start it:

```bash
vix service start
```

### Binary is missing

Build the app:

```bash
vix build
```

or for full target builds:

```bash
vix build --build-target all
```

Then run:

```bash
vix doctor production
```

### App is not running

Check service status:

```bash
vix service status
```

Start or restart:

```bash
vix service start
vix service restart
```

### Nginx proxy is missing

Initialize the proxy:

```bash
vix proxy nginx init
```

Validate it:

```bash
vix proxy nginx check
```

Reload Nginx:

```bash
vix proxy nginx reload
```

### TLS is missing

After configuring a domain, run:

```bash
vix proxy nginx certbot
```

Then check again:

```bash
vix doctor production
```

### Health checks fail

Check local health:

```bash
vix health local
```

Check public health:

```bash
vix health public
```

Then inspect logs:

```bash
vix logs
vix logs errors
```

## Recommended workflows

### Check a new machine

```bash
vix doctor
vix doctor toolchain
```

### Check after upgrade

```bash
vix doctor
vix doctor --online
vix doctor toolchain
```

### Check before serious development

```bash
vix doctor
vix info
vix build
vix check
```

### Check production app

```bash
vix doctor production
vix health local
vix health public
vix logs
```

### Check production app in CI

```bash
vix doctor production --json
```

## Best practices

Run `vix doctor` after installing Vix.

Run `vix doctor toolchain` when builds behave strangely.

Run `vix doctor --online` when you suspect the CLI is outdated.

Run `vix doctor production` after deploying a backend app.

Run `vix doctor production --json` in automation.

Keep production configuration in `vix.json`.

Use the same Vix installation for the CLI and CMake package.

Do not ignore production readiness warnings.

## Related commands

| Command               | Purpose                                        |
| --------------------- | ---------------------------------------------- |
| `vix info`            | Show paths, caches, and local state.           |
| `vix build`           | Build the project.                             |
| `vix check`           | Validate project health.                       |
| `vix upgrade --check` | Check upgrade target and download info.        |
| `vix upgrade`         | Upgrade Vix.                                   |
| `vix registry sync`   | Refresh registry metadata.                     |
| `vix install`         | Install project dependencies.                  |
| `vix reset`           | Reset generated state and dependencies.        |
| `vix service`         | Manage production systemd services.            |
| `vix proxy nginx`     | Manage Nginx reverse proxy config.             |
| `vix health`          | Run local, public, or WebSocket health checks. |
| `vix logs`            | Inspect app and proxy logs.                    |
| `vix deploy`          | Deploy a production app.                       |

## Next step

Inspect local Vix paths and state.

[Open the vix info guide](/cli/info)
