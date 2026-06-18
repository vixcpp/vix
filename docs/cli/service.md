# vix service

`vix service` manages a Vix application as a Linux systemd service.

Use it when you want to install, start, stop, restart, inspect, or health-check a production Vix app.

```bash
vix service install
```

## Overview

`vix service` is the production service command for Vix.

It helps turn a built Vix application into a managed Linux service.

It can:

- generate a systemd service
- install the service under `/etc/systemd/system`
- enable the service on boot
- start the service
- stop the service
- restart the service
- show service status
- show recent service logs
- run configured health checks
- warn when the service points to an old binary
- warn when the service uses a different Vix CLI installation

This command is part of the Vix production workflow.

The goal is to avoid writing systemd units manually for every Vix backend.

## Platform support

`vix service` is currently supported on:

```txt
Linux + systemd
```

It uses commands such as:

```txt
systemctl
journalctl
sudo
```

On unsupported platforms, Vix reports that `vix service` is currently supported on Linux/systemd only.

## Usage

```bash
vix service <command>
```

## Commands

| Command   | Purpose                                 |
| --------- | --------------------------------------- |
| `install` | Generate and install a systemd service. |
| `start`   | Start the service.                      |
| `stop`    | Stop the service.                       |
| `restart` | Restart the service.                    |
| `status`  | Show systemd service status.            |
| `logs`    | Show recent service logs.               |
| `health`  | Run configured HTTP health checks.      |

## Basic examples

```bash
# Build the app first
vix build

# Install the systemd service
vix service install

# Start the service
vix service start

# Show service status
vix service status

# Show recent service logs
vix service logs

# Restart the service
vix service restart

# Run configured health checks
vix service health

# Stop the service
vix service stop
```

## Typical production workflow

A simple production workflow looks like this:

```bash
vix build --preset release
vix service install
vix service start
vix service status
vix service health
vix doctor production
```

After a new deployment:

```bash
vix build --preset release
vix service restart
vix service status
vix service health
```

## How Vix detects the app

`vix service` runs from the current project directory.

It detects the app name from:

```txt
vix.json name
*.vix file name
current directory name
```

For example, inside:

```txt
/home/user/PulseGrid
```

Vix may detect:

```txt
PulseGrid
```

The default systemd service name becomes:

```txt
pulsegrid.service
```

## How Vix detects the binary

`vix service` looks for a build directory such as:

```txt
build-ninja
build-release
build
cmake-build-debug
cmake-build-release
```

Then it tries to find the binary matching the app name.

For an app named:

```txt
PulseGrid
```

it may look for:

```txt
build-ninja/PulseGrid
```

If the binary is missing, `vix service install` fails and tells you to build first.

```bash
vix build
```

## Install a service

Run:

```bash
vix service install
```

This command:

1. loads the service config
2. detects the app name
3. detects the executable path
4. detects the current user
5. detects the Vix package path when possible
6. renders a systemd unit
7. writes it to a temporary file
8. copies it to `/etc/systemd/system`
9. runs `systemctl daemon-reload`
10. enables the service on boot

You may be asked for `sudo` permission.

## Generated systemd unit

A generated service looks like this:

```ini
[Unit]
Description=PulseGrid
After=network.target

[Service]
User=gaspard
WorkingDirectory=/home/gaspard/PulseGrid
Environment=VIX_CLI_PATH=/home/gaspard/.local/bin/vix
Environment=Vix_DIR=/home/gaspard/.local
Environment=CMAKE_PREFIX_PATH=/home/gaspard/.local
ExecStart=/home/gaspard/PulseGrid/build-ninja/PulseGrid
Restart=always
RestartSec=3
LimitNOFILE=65535

[Install]
WantedBy=multi-user.target
```

The exact values depend on your project and `vix.json`.

## Service configuration in `vix.json`

You can configure the generated service in `vix.json`.

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
      "limit_nofile": 65535,
      "health_local": "http://127.0.0.1:8080/",
      "health_public": "https://pulsegrid.example.com/",
      "health_timeout_ms": 2000
    }
  }
}
```

## Service config fields

| Field                                  | Purpose                                                          |
| -------------------------------------- | ---------------------------------------------------------------- |
| `production.service.name`              | systemd service name. `.service` is added if missing.            |
| `production.service.user`              | Linux user used to run the service.                              |
| `production.service.working_dir`       | Working directory for the app.                                   |
| `production.service.exec`              | Executable path. Relative paths are resolved from `working_dir`. |
| `production.service.restart`           | systemd restart policy. Default: `always`.                       |
| `production.service.restart_sec`       | Restart delay in seconds. Default: `3`.                          |
| `production.service.limit_nofile`      | File descriptor limit. Default: `65535`.                         |
| `production.service.health_local`      | Local health check URL.                                          |
| `production.service.health_public`     | Public health check URL.                                         |
| `production.service.health_timeout_ms` | Health check timeout in milliseconds. Default: `2000`.           |

## Default service config

If `vix.json` does not define `production.service`, Vix uses defaults.

| Value                 | Default                                             |
| --------------------- | --------------------------------------------------- |
| app name              | `vix.json` name, `.vix` filename, or current folder |
| service name          | lowercase app name + `.service`                     |
| working directory     | current project directory                           |
| executable            | detected build output                               |
| user                  | current Linux user                                  |
| restart               | `always`                                            |
| restart delay         | `3` seconds                                         |
| file descriptor limit | `65535`                                             |

## Start the service

Run:

```bash
vix service start
```

This maps to:

```bash
sudo systemctl start <service>
```

Example:

```bash
vix service start
```

Expected output shape:

```txt
service start: pulsegrid.service
```

## Stop the service

Run:

```bash
vix service stop
```

This maps to:

```bash
sudo systemctl stop <service>
```

Use it when you want to stop the production app.

## Restart the service

Run:

```bash
vix service restart
```

This maps to:

```bash
sudo systemctl restart <service>
```

Use it after rebuilding or deploying:

```bash
vix build --preset release
vix service restart
vix service status
vix service health
```

## Show service status

Run:

```bash
vix service status
```

This maps to:

```bash
systemctl status <service>
```

Before showing status, Vix can warn if the installed service appears stale.

For example, it can warn when:

- the service executable points to a different build path
- the service uses a different Vix CLI installation

## Show service logs

Run:

```bash
vix service logs
```

This maps to:

```bash
journalctl -u <service> -n 120 --no-pager
```

Use it when the app fails to start or exits unexpectedly.

For live logs and broader production log inspection, use:

```bash
vix logs
```

when available.

## Run service health checks

Run:

```bash
vix service health
```

This command reads configured health URLs from `vix.json`.

Supported fields:

```json
{
  "production": {
    "service": {
      "health_local": "http://127.0.0.1:8080/",
      "health_public": "https://pulsegrid.example.com/",
      "health_timeout_ms": 2000
    }
  }
}
```

Then Vix sends HTTP GET requests and reports whether they pass.

## Local health check

Local health checks verify the app directly on the server.

Example:

```json
{
  "production": {
    "service": {
      "health_local": "http://127.0.0.1:8080/"
    }
  }
}
```

Run:

```bash
vix service health
```

Expected output shape:

```txt
Service Health
Service: pulsegrid.service
Timeout: 2000ms
Health local: http://127.0.0.1:8080/
local health check passed: HTTP 200
```

## Public health check

Public health checks verify the app through its public URL.

Example:

```json
{
  "production": {
    "service": {
      "health_public": "https://pulsegrid.example.com/"
    }
  }
}
```

Run:

```bash
vix service health
```

Expected output shape:

```txt
Health public: https://pulsegrid.example.com/
public health check passed: HTTP 200
```

## Health check failures

If a health check fails, Vix shows the failed check, HTTP status when available, and the error.

Example shape:

```txt
local health check failed
HTTP status: 500
```

or:

```txt
public health check failed
connection refused
```

If no health check is configured, Vix tells you what to add:

```txt
No health check configured.
Add production.service.health_local or production.service.health_public to vix.json.
```

## Vix installation environment

When installing a service, Vix can include environment variables that help the service use the right Vix installation.

Example:

```ini
Environment=VIX_CLI_PATH=/home/user/.local/bin/vix
Environment=Vix_DIR=/home/user/.local
Environment=CMAKE_PREFIX_PATH=/home/user/.local
```

This is important when a machine has multiple Vix builds or installations.

## Stale service warnings

`vix service status` can warn when the installed systemd service no longer matches the current project.

It can detect cases such as:

```txt
service ExecStart points to an old build directory
service VIX_CLI_PATH points to a different Vix installation
```

Example warning:

```txt
Service executable points to a different build path.
Fix: run `vix service install` then `vix service restart`.
```

Another example:

```txt
Service uses a different Vix CLI installation.
Fix: update production.service.environment.VIX_CLI_PATH, then run `vix service install`.
```

These warnings help avoid confusing production bugs where the service runs an older binary or a different Vix installation than the shell.

## Relationship with `vix doctor production`

`vix service` creates and manages the systemd service.

`vix doctor production` inspects whether the production setup is healthy.

After installing or changing a service, run:

```bash
vix doctor production
```

This can detect:

- service installed
- service running
- binary exists
- restart policy
- working directory
- environment variables
- local health
- public health
- production readiness score

## Relationship with `vix deploy`

`vix deploy` can use the service layer as part of deployment.

A deployment flow can be:

```bash
vix deploy
```

Internally, a production deploy can build, restart the service, verify status, run health checks, and show logs on failure.

`vix service` remains useful when you want direct control.

## Relationship with `vix proxy nginx`

`vix service` manages the app process.

`vix proxy nginx` manages the reverse proxy.

A complete production setup usually needs both:

```bash
vix service install
vix service start
vix proxy nginx init
vix proxy nginx check
vix proxy nginx reload
vix doctor production
```

## Recommended production setup

A good initial setup is:

```bash
vix build --preset release
vix service install
vix service start
vix service status
vix service health
vix doctor production
```

If the app is public:

```bash
vix proxy nginx init
vix proxy nginx check
vix proxy nginx reload
vix doctor production
```

If TLS is needed:

```bash
vix proxy nginx certbot
vix doctor production
```

## Options

`vix service` currently uses subcommands.

```bash
vix service <command>
```

| Command      | Description                             |
| ------------ | --------------------------------------- |
| `install`    | Generate and install a systemd service. |
| `start`      | Start the systemd service.              |
| `stop`       | Stop the systemd service.               |
| `restart`    | Restart the systemd service.            |
| `status`     | Show systemd service status.            |
| `logs`       | Show recent service logs.               |
| `health`     | Run configured HTTP health checks.      |
| `-h, --help` | Show help.                              |

## Environment variables

| Variable            | Purpose                                                                  |
| ------------------- | ------------------------------------------------------------------------ |
| `USER`              | Used to detect the current user if `production.service.user` is not set. |
| `Vix_DIR`           | Used to detect the Vix CMake package path.                               |
| `CMAKE_PREFIX_PATH` | Used to detect the Vix package prefix.                                   |
| `VIX_CLI_PATH`      | Stored in the service to remember the CLI path used during install.      |

## Common workflows

### Install and start a service

```bash
vix build --preset release
vix service install
vix service start
vix service status
```

### Restart after a rebuild

```bash
vix build --preset release
vix service restart
vix service health
```

### Check service logs

```bash
vix service logs
```

### Check local and public health

```bash
vix service health
```

### Reinstall service after changing `vix.json`

```bash
vix service install
vix service restart
vix service status
```

### Verify complete production state

```bash
vix doctor production
```

## Common mistakes

### Installing before building

Wrong:

```bash
vix service install
```

when the binary does not exist.

Correct:

```bash
vix build --preset release
vix service install
```

### Running outside the project directory

Wrong:

```bash
cd ~
vix service install
```

Correct:

```bash
cd /path/to/myapp
vix service install
```

### Forgetting to restart after rebuilding

Wrong:

```bash
vix build --preset release
vix service status
```

Correct:

```bash
vix build --preset release
vix service restart
vix service status
```

### Expecting `vix service health` to work without health config

Wrong:

```bash
vix service health
```

without `health_local` or `health_public`.

Correct:

```json
{
  "production": {
    "service": {
      "health_local": "http://127.0.0.1:8080/"
    }
  }
}
```

Then run:

```bash
vix service health
```

### Editing `vix.json` but not reinstalling the service

If you change service fields such as `user`, `exec`, `working_dir`, `restart`, or `limit_nofile`, reinstall the service:

```bash
vix service install
vix service restart
```

### Ignoring stale service warnings

If `vix service status` warns that the service points to an old build path, reinstall and restart:

```bash
vix service install
vix service restart
```

## Troubleshooting

### Executable not found

Build first:

```bash
vix build --preset release
```

Then install:

```bash
vix service install
```

If your binary has a custom name or path, configure it:

```json
{
  "production": {
    "service": {
      "exec": "build-release/my_app"
    }
  }
}
```

### Service fails to start

Check status:

```bash
vix service status
```

Check logs:

```bash
vix service logs
```

Common causes:

- binary missing
- wrong `ExecStart`
- wrong working directory
- missing environment variables
- permission issue
- app crashes immediately
- port already in use

### Service uses the wrong binary

Run:

```bash
vix service status
```

If Vix warns about a different build path, reinstall:

```bash
vix service install
vix service restart
```

### Service uses the wrong Vix installation

Run:

```bash
vix service status
```

or:

```bash
vix doctor toolchain
```

Make sure `Vix_DIR`, `CMAKE_PREFIX_PATH`, and `VIX_CLI_PATH` point to the correct installation.

Then reinstall:

```bash
vix service install
vix service restart
```

### Health check fails

Run:

```bash
vix service status
vix service logs
vix service health
```

Check that the app is listening on the expected port.

If using Nginx, also run:

```bash
vix proxy nginx check
vix doctor production
```

### Permission denied

`vix service install`, `start`, `stop`, and `restart` use `sudo systemctl` or write under `/etc/systemd/system`.

Make sure your user can use `sudo`.

### systemd daemon did not reload

Reinstall the service:

```bash
vix service install
```

This runs:

```bash
sudo systemctl daemon-reload
```

Then restart:

```bash
vix service restart
```

## Best practices

Build before installing the service.
Use release builds for production services.
Keep service configuration in `vix.json`.

Use explicit `production.service.exec` for production apps.

Use `health_local` for local health checks.

Use `health_public` for public health checks.

Run `vix service health` after every restart.

Run `vix doctor production` after every production setup change.

Reinstall the service after changing service config.

Use `vix logs` for broader production log inspection when available.

## Related commands

| Command                 | Purpose                                   |
| ----------------------- | ----------------------------------------- |
| `vix build`             | Build the production binary.              |
| `vix run`               | Run the app manually.                     |
| `vix dev`               | Run the app in development mode.          |
| `vix doctor production` | Inspect production readiness.             |
| `vix doctor toolchain`  | Check CLI and CMake package consistency.  |
| `vix proxy nginx`       | Manage Nginx reverse proxy configuration. |
| `vix health`            | Run production health checks.             |
| `vix logs`              | Inspect production logs.                  |
| `vix deploy`            | Deploy a production app.                  |

## Next step

Check production readiness.

[Open the production doctor guide](/cli/doctor)
