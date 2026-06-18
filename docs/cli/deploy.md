# vix deploy

`vix deploy` runs a production deployment workflow for a Vix application.

Use it when you want to pull the latest code, build the app, run tests, restart the service, verify health, validate proxy configuration, reload Nginx, and optionally rollback on failure.

```bash
vix deploy
```

## Overview

`vix deploy` is the production deployment command for Vix.

It can:

- pull the latest code from Git
- build the production app
- run tests before restart
- restart the systemd service
- verify service status
- run health checks
- validate Nginx proxy config
- reload Nginx
- show logs on failure
- run repeated error analysis on failure
- rollback to the previous Git revision when enabled
- run in dry-run mode
- skip pull or tests from the CLI

This command is designed for Linux server deployments where a Vix app is already configured with:

```txt
vix service
vix health
vix proxy nginx
vix logs
```

## Platform support

`vix deploy` is currently supported on:

```txt
Linux
```

On unsupported platforms, Vix reports:

```txt
vix deploy is currently supported on Linux only.
```

## Usage

```bash
vix deploy [options]
```

## Basic examples

```bash
# Run the configured deployment
vix deploy

# Print steps without executing them
vix deploy --dry-run

# Deploy without git pull
vix deploy --no-pull

# Deploy without tests
vix deploy --no-tests

# Show more details
vix deploy --verbose
```

## What `vix deploy` does

A configured deployment can run this flow:

```txt
print deploy summary
capture current Git revision for rollback
git pull origin <branch>
run build command
run test command
restart service
check service status
run health checks
check Nginx proxy config
reload Nginx
print deployment completed
```

The exact steps depend on `vix.json`.

## Configuration source

`vix deploy` reads deployment configuration from:

```txt
vix.json
```

under:

```txt
production.deploy
```

Example:

```json
{
  "name": "PulseGrid",
  "production": {
    "deploy": {
      "pull": true,
      "branch": "main",
      "build": "vix build --preset release",
      "tests": true,
      "test_command": "vix tests",
      "service": "pulsegrid",
      "health_local": true,
      "health_public": true,
      "proxy_check": true,
      "proxy_reload": true,
      "logs_on_failure": true,
      "log_lines": 80,
      "rollback": true
    }
  }
}
```

## Deploy config fields

| Field                               | Purpose                                                      |
| ----------------------------------- | ------------------------------------------------------------ |
| `production.deploy.pull`            | Whether to run `git pull origin <branch>`.                   |
| `production.deploy.branch`          | Git branch to pull.                                          |
| `production.deploy.build`           | Build command to run.                                        |
| `production.deploy.tests`           | Whether to run tests.                                        |
| `production.deploy.test_command`    | Test command to run.                                         |
| `production.deploy.service`         | systemd service name managed by `vix service`.               |
| `production.deploy.health_local`    | Whether deployment should run local health checks.           |
| `production.deploy.health_public`   | Whether deployment should run public health checks.          |
| `production.deploy.proxy_check`     | Whether deployment should validate Nginx proxy config.       |
| `production.deploy.proxy_reload`    | Whether deployment should reload Nginx after validation.     |
| `production.deploy.logs_on_failure` | Whether to show failure logs automatically.                  |
| `production.deploy.log_lines`       | Number of log lines used by failure diagnostics.             |
| `production.deploy.rollback`        | Whether to rollback to the previous Git revision on failure. |

## Default behavior

If some fields are missing, Vix uses safe defaults.

Common defaults are:

```txt
app name: project name
service: app name
branch: main
build command: vix build --preset release
test command: vix tests
log lines: 80
```

Boolean fields are controlled by your `vix.json`.

CLI flags can override selected behavior:

```bash
vix deploy --no-pull
vix deploy --no-tests
```

## Deploy summary

Before executing, Vix prints a deployment summary.

Example output shape:

```txt
Deploy
App: PulseGrid
Branch: main
Pull: yes
Build: vix build --preset release
Tests: enabled
Service: pulsegrid
Health local: enabled
Health public: enabled
Proxy check: enabled
Proxy reload: enabled
Logs on failure: enabled
Rollback: enabled
Dry run: no
Verbose: no
```

This makes the deployment plan visible before the command runs.

## Dry run

Use `--dry-run` to print the deployment steps without executing them.

```bash
vix deploy --dry-run
```

This is useful before enabling production deployment.

Example output shape:

```txt
Deploy
App: PulseGrid
Branch: main
Pull: yes
Build: vix build --preset release
Tests: enabled
Service: pulsegrid
Health local: enabled
Health public: enabled
Proxy check: enabled
Proxy reload: enabled
Logs on failure: enabled
Rollback: enabled
Dry run: yes

Pull
Command: git pull origin 'main'

Build
Command: vix build --preset release

Tests
Command: vix tests

Restart Service
Command: vix service restart

Service Status
Command: vix service status

Health Check
Command: vix health

Proxy Check
Command: vix proxy nginx check

Proxy Reload
Command: vix proxy nginx reload

deployment completed
```

Dry run helps verify the deployment plan without changing the server.

## Pull step

When enabled, Vix runs:

```bash
git pull origin <branch>
```

Example:

```bash
git pull origin 'main'
```

Config:

```json
{
  "production": {
    "deploy": {
      "pull": true,
      "branch": "main"
    }
  }
}
```

Skip pull for one deployment:

```bash
vix deploy --no-pull
```

Use `--no-pull` when the code is already updated manually.

## Build step

Vix runs the configured build command.

Example:

```json
{
  "production": {
    "deploy": {
      "build": "vix build --preset release"
    }
  }
}
```

This executes:

```bash
vix build --preset release
```

A stronger build command can include a target:

```json
{
  "production": {
    "deploy": {
      "build": "vix build --preset release --build-target all"
    }
  }
}
```

Use the build command that matches your production app.

## Test step

When tests are enabled, Vix runs the configured test command.

Example:

```json
{
  "production": {
    "deploy": {
      "tests": true,
      "test_command": "vix tests"
    }
  }
}
```

Skip tests for one deployment:

```bash
vix deploy --no-tests
```

Use `--no-tests` only when you intentionally want to bypass the test step.

## Service restart step

After a successful build and optional tests, Vix restarts the service:

```bash
vix service restart
```

Then it checks service status:

```bash
vix service status
```

The service name must be configured.

Example:

```json
{
  "production": {
    "deploy": {
      "service": "pulsegrid"
    }
  }
}
```

If the service name is missing, Vix reports:

```txt
Missing deployment service name.
Fix: add production.deploy.service to vix.json
```

## Health check step

If either `health_local` or `health_public` is enabled, Vix runs:

```bash
vix health
```

Config:

```json
{
  "production": {
    "deploy": {
      "health_local": true,
      "health_public": true
    },
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/",
      "public": "https://pulsegrid.example.com/"
    }
  }
}
```

Use health checks to catch broken deployments immediately after restart.

## Proxy check step

If `proxy_check` or `proxy_reload` is enabled, Vix validates the Nginx proxy config:

```bash
vix proxy nginx check
```

Config:

```json
{
  "production": {
    "deploy": {
      "proxy_check": true
    }
  }
}
```

This is useful when deployment depends on a correct public proxy configuration.

## Proxy reload step

If `proxy_reload` is enabled, Vix reloads Nginx after a successful proxy check:

```bash
vix proxy nginx reload
```

Config:

```json
{
  "production": {
    "deploy": {
      "proxy_reload": true
    }
  }
}
```

Reload is only useful when the proxy configuration may change during deployment.

## Logs on failure

If `logs_on_failure` is enabled, Vix prints failure logs automatically.

Config:

```json
{
  "production": {
    "deploy": {
      "logs_on_failure": true,
      "log_lines": 80
    }
  }
}
```

On failure, Vix runs a command shaped like:

```bash
vix logs errors --repeated -n 80
```

This helps you see repeated production errors without manually switching commands.

## Rollback

If `rollback` is enabled, Vix captures the current Git revision before deployment:

```bash
git rev-parse HEAD
```

If a later step fails, Vix attempts to rollback:

```txt
git reset --hard <previous-ref>
run build command again
vix service restart
vix service status
```

Config:

```json
{
  "production": {
    "deploy": {
      "rollback": true
    }
  }
}
```

Rollback is useful when a pulled change builds or starts incorrectly.

## Rollback limitations

Rollback depends on Git state.

Rollback may be skipped or fail when:

- the current Git revision could not be captured
- the project is not a Git repository
- local changes prevent clean reset
- the previous revision no longer builds
- service restart fails after rollback

If rollback fails, Vix prints:

```txt
rollback failed
Fix: inspect the repository and service manually
```

Rollback is a safety net, not a replacement for backups or release discipline.

## Failure behavior

If a step fails, deployment stops.

Typical failure points:

| Step         | Failure message          | Fix                                        |
| ------------ | ------------------------ | ------------------------------------------ |
| Pull         | `git pull failed`        | Check repository state and branch.         |
| Build        | `build failed`           | Run the configured build command manually. |
| Tests        | `tests failed`           | Run the configured test command manually.  |
| Restart      | `service restart failed` | Run `vix service restart`.                 |
| Status       | `service is not active`  | Run `vix service status`.                  |
| Health       | `health check failed`    | Run `vix health`.                          |
| Proxy check  | `proxy check failed`     | Run `vix proxy nginx check`.               |
| Proxy reload | `proxy reload failed`    | Run `vix proxy nginx reload`.              |

If rollback is enabled, Vix attempts rollback before showing failure logs.

If logs on failure is enabled, Vix runs:

```bash
vix logs errors --repeated -n <log_lines>
```

## Full production deploy config

```json
{
  "name": "PulseGrid",
  "production": {
    "deploy": {
      "pull": true,
      "branch": "main",
      "build": "vix build --preset release",
      "tests": true,
      "test_command": "vix tests",
      "service": "pulsegrid",
      "health_local": true,
      "health_public": true,
      "proxy_check": true,
      "proxy_reload": true,
      "logs_on_failure": true,
      "log_lines": 80,
      "rollback": true
    },
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/",
      "public": "https://pulsegrid.example.com/"
    },
    "logs": {
      "service": "pulsegrid",
      "nginx_access": "/var/log/nginx/pulsegrid.access.log",
      "nginx_error": "/var/log/nginx/pulsegrid.error.log"
    },
    "proxy": {
      "domain": "pulsegrid.example.com",
      "http": {
        "port": 8080
      },
      "tls": {
        "enabled": true
      }
    }
  }
}
```

## Minimal deploy config

For a simple local server deployment:

```json
{
  "name": "PulseGrid",
  "production": {
    "deploy": {
      "pull": false,
      "build": "vix build --preset release",
      "tests": false,
      "service": "pulsegrid",
      "health_local": true,
      "logs_on_failure": true
    },
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/"
    }
  }
}
```

Then run:

```bash
vix deploy
```

## Public HTTPS deploy config

For an app behind Nginx and TLS:

```json
{
  "name": "PulseGrid",
  "production": {
    "deploy": {
      "pull": true,
      "branch": "main",
      "build": "vix build --preset release",
      "tests": true,
      "test_command": "vix tests",
      "service": "pulsegrid",
      "health_local": true,
      "health_public": true,
      "proxy_check": true,
      "proxy_reload": true,
      "logs_on_failure": true,
      "log_lines": 120,
      "rollback": true
    },
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/",
      "public": "https://pulsegrid.example.com/"
    },
    "proxy": {
      "domain": "pulsegrid.example.com",
      "http": {
        "port": 8080
      },
      "tls": {
        "enabled": true
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

## WebSocket deploy config

For an app with WebSocket production checks:

```json
{
  "name": "PulseGrid",
  "production": {
    "deploy": {
      "pull": true,
      "branch": "main",
      "build": "vix build --preset release",
      "tests": true,
      "test_command": "vix tests",
      "service": "pulsegrid",
      "health_local": true,
      "health_public": true,
      "proxy_check": true,
      "proxy_reload": true,
      "logs_on_failure": true,
      "rollback": true
    },
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/",
      "public": "https://pulsegrid.example.com/",
      "websocket": "wss://pulsegrid.example.com/ws"
    },
    "proxy": {
      "domain": "pulsegrid.example.com",
      "http": {
        "port": 8080
      },
      "websocket": {
        "enabled": true,
        "path": "/ws",
        "port": 9090
      },
      "tls": {
        "enabled": true
      }
    }
  }
}
```

`vix deploy` runs `vix health` when health is enabled.

That means configured WebSocket health can be checked as part of the normal health command.

## Options

| Option       | Description                                      |
| ------------ | ------------------------------------------------ |
| `--dry-run`  | Print deployment steps without executing them.   |
| `--verbose`  | Print additional execution details.              |
| `-v`         | Alias for `--verbose`.                           |
| `--no-pull`  | Skip `git pull` even when enabled in `vix.json`. |
| `--no-tests` | Skip tests even when enabled in `vix.json`.      |
| `-h, --help` | Show help.                                       |

## Commands run by deploy

Depending on config, Vix can run:

```bash
git pull origin '<branch>'
<build command>
<test command>
vix service restart
vix service status
vix health
vix proxy nginx check
vix proxy nginx reload
vix logs errors --repeated -n <lines>
git reset --hard <previous-ref>
```

In `--dry-run`, commands are printed but not executed.

## Relationship with `vix env`

Validate environment before deployment:

```bash
vix env check --production
```

This catches missing production variables before restarting the service.

## Relationship with `vix db`

For database-backed apps, check and backup the database before deployment:

```bash
vix db status
vix db backup
```

A safer deployment flow can be:

```bash
vix env check --production
vix db status
vix db backup
vix deploy
```

## Relationship with `vix service`

`vix deploy` restarts and verifies the service using:

```bash
vix service restart
vix service status
```

Make sure service installation is already done:

```bash
vix service install
vix service start
```

## Relationship with `vix health`

`vix deploy` can run:

```bash
vix health
```

when `health_local` or `health_public` is enabled.

Configure health endpoints under:

```txt
production.health
```

## Relationship with `vix proxy nginx`

`vix deploy` can run:

```bash
vix proxy nginx check
vix proxy nginx reload
```

when proxy validation or reload is enabled.

Configure proxy under:

```txt
production.proxy
```

## Relationship with `vix logs`

When `logs_on_failure` is enabled, Vix runs:

```bash
vix logs errors --repeated -n <log_lines>
```

This helps diagnose failure immediately.

## Relationship with `vix doctor production`

After deployment, inspect full readiness:

```bash
vix doctor production
```

A strong production workflow is:

```bash
vix env check --production
vix db status
vix deploy
vix doctor production
```

## Recommended workflows

### First deployment setup

```bash
vix env check --production
vix build --preset release
vix service install
vix service start
vix proxy nginx init
vix health
vix doctor production
```

Then configure `production.deploy`.

After that:

```bash
vix deploy
```

### Safe deployment

```bash
vix env check --production
vix db status
vix db backup
vix deploy
vix doctor production
```

### Preview deployment

```bash
vix deploy --dry-run
```

### Deploy without pulling

```bash
vix deploy --no-pull
```

### Deploy without tests

```bash
vix deploy --no-tests
```

### Debug failed deployment

```bash
vix logs errors --repeated
vix service status
vix health
vix proxy nginx check
```

## Common mistakes

### Running deploy before installing the service

Wrong:

```bash
vix deploy
```

before service setup.

Correct:

```bash
vix service install
vix service start
vix deploy
```

### Missing deployment service name

Wrong:

```json
{
  "production": {
    "deploy": {
      "build": "vix build --preset release"
    }
  }
}
```

Correct:

```json
{
  "production": {
    "deploy": {
      "service": "pulsegrid",
      "build": "vix build --preset release"
    }
  }
}
```

### Enabling health checks without health config

Wrong:

```json
{
  "production": {
    "deploy": {
      "health_local": true
    }
  }
}
```

without:

```json
{
  "production": {
    "health": {
      "local": "http://127.0.0.1:8080/"
    }
  }
}
```

Correct: configure `production.health`.

### Enabling proxy checks without proxy config

Wrong:

```json
{
  "production": {
    "deploy": {
      "proxy_check": true
    }
  }
}
```

without `production.proxy`.

Correct: configure proxy first:

```bash
vix proxy nginx init
vix proxy nginx check
```

### Enabling rollback without Git

Rollback needs Git.

If the current Git revision cannot be captured, rollback may be skipped.

Check:

```bash
git rev-parse HEAD
```

### Forgetting database backup

For SQLite apps, do not deploy risky changes without:

```bash
vix db backup
```

### Treating rollback as a database rollback

`vix deploy` rollback resets Git and restarts the service.

It does not rollback database migrations or restore database backups.

Back up the database separately before deployment.

## Troubleshooting

### Git pull failed

Run:

```bash
git status
git pull origin main
```

Common causes:

- local uncommitted changes
- wrong branch
- remote authentication failure
- merge conflict
- network failure

### Build failed

Run the configured build command manually.

Example:

```bash
vix build --preset release
```

Then inspect errors.

### Tests failed

Run the configured test command manually.

Example:

```bash
vix tests
```

### Service restart failed

Run:

```bash
vix service restart
vix service status
vix service logs
```

### Service is not active

Run:

```bash
vix service status
vix logs app --errors
```

### Health check failed

Run:

```bash
vix health
vix logs errors --repeated
```

If local health fails, inspect app/service.

If public health fails, inspect proxy and TLS.

### Proxy check failed

Run:

```bash
vix proxy nginx check
```

Then fix `production.proxy` or regenerate proxy config.

### Proxy reload failed

Run:

```bash
sudo nginx -t
vix proxy nginx reload
```

### Rollback failed

Inspect manually:

```bash
git status
vix build --preset release
vix service status
vix logs errors --repeated
```

## Best practices

Run `vix deploy --dry-run` before trusting a new deployment config.

Keep deployment config in `vix.json`.

Use `--no-pull` only when code is already updated.

Use `--no-tests` only when intentionally bypassing tests.

Enable `logs_on_failure`.

Enable rollback for Git-based deployments.

Back up SQLite databases before deployment.

Run `vix env check --production` before deployment.

Run `vix doctor production` after deployment.

Keep health checks lightweight and reliable.

Use explicit build and test commands.

Do not treat Git rollback as database rollback.

## Related commands

| Command                      | Purpose                                     |
| ---------------------------- | ------------------------------------------- |
| `vix env check --production` | Validate production env before deployment.  |
| `vix db status`              | Check database readiness.                   |
| `vix db backup`              | Back up SQLite database before deployment.  |
| `vix build`                  | Build the production binary.                |
| `vix tests`                  | Run project tests.                          |
| `vix service`                | Manage production systemd service.          |
| `vix health`                 | Verify local, public, and WebSocket health. |
| `vix proxy nginx`            | Validate and reload Nginx proxy.            |
| `vix logs`                   | Inspect production errors.                  |
| `vix doctor production`      | Inspect complete production readiness.      |

## Next step

Check production readiness.

[Open the production doctor guide](/cli/doctor)
