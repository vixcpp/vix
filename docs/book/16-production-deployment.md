# Production deployment

In the previous chapter, you learned P2P.
Now you will learn how to deploy a Vix application in production.

```txt
browser
  -> HTTPS
  -> Nginx
  -> Vix app on localhost
  -> systemd
```

## Why production deployment matters

During development, you usually run:

```bash
vix dev
```

Production is different.

A production application needs:

- A release build
- A stable working directory
- A systemd service
- Environment validation
- Logs
- A reverse proxy
- HTTPS
- Health checks
- WebSocket checks when needed
- A safe deployment workflow

Vix now provides commands for these production tasks.

Instead of manually writing everything from scratch every time, the production workflow becomes:

```bash
vix env check --production
vix build --preset release
vix check --tests
vix service init
vix proxy nginx init
vix health
vix logs
vix deploy
```

## Production architecture

A normal Vix production setup looks like this:

```txt
Internet
  -> Nginx
  -> 127.0.0.1:8080
  -> Vix app
  -> systemd
```

Nginx handles public HTTP and HTTPS.

The Vix app listens locally.

systemd keeps the app alive.

For WebSocket apps:

```txt
Internet
  -> Nginx WebSocket location
  -> 127.0.0.1:9090
  -> Vix WebSocket server
```

## Development vs production

| Development       | Production                |
| ----------------- | ------------------------- |
| `vix dev`         | `vix service`             |
| Hot reload        | Stable systemd process    |
| Terminal logs     | `vix logs`                |
| Local browser     | Nginx reverse proxy       |
| Manual restart    | systemd restart policy    |
| Local env         | production env validation |
| Manual checks     | `vix health`              |
| Manual deployment | `vix deploy`              |

The important idea is simple:

```txt
development optimizes iteration
production optimizes stability
```

## The production command set

Vix includes production-oriented commands:

| Command                      | Purpose                                       |
| ---------------------------- | --------------------------------------------- |
| `vix env check --production` | Validate production environment configuration |
| `vix service`                | Manage the systemd service                    |
| `vix proxy nginx`            | Generate, check, reload, and configure Nginx  |
| `vix health`                 | Check local, public, and WebSocket endpoints  |
| `vix logs`                   | Read app, proxy, and error logs               |
| `vix ws check`               | Check WebSocket endpoint behavior             |
| `vix deploy`                 | Run the deployment workflow                   |

These commands make production less manual and more predictable.

## Production configuration

Production behavior should be described in `vix.json`.

Example:

```json
{
  "production": {
    "service": {
      "name": "myapp",
      "user": "vix",
      "working_dir": "/home/vix/apps/myapp",
      "command": "vix run",
      "env_file": "/home/vix/apps/myapp/.env"
    },
    "proxy": {
      "domain": "example.com",
      "http_port": 8080,
      "websocket": {
        "enabled": true,
        "path": "/ws",
        "port": 9090
      },
      "tls": {
        "enabled": true,
        "certificate": "/etc/letsencrypt/live/example.com/fullchain.pem",
        "certificate_key": "/etc/letsencrypt/live/example.com/privkey.pem"
      }
    },
    "health": {
      "service": "myapp",
      "local": "http://127.0.0.1:8080/health",
      "public": "https://example.com/health",
      "websocket": "wss://example.com/ws"
    },
    "logs": {
      "service": "myapp",
      "nginx_access": "/var/log/nginx/example.com.access.log",
      "nginx_error": "/var/log/nginx/example.com.error.log"
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

This config gives Vix enough information to manage the service, proxy, logs, health checks, and deployments.

## Prepare the server

Install the required system packages:

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  git nginx certbot python3-certbot-nginx
```

Depending on your app, you may also need:

```bash
sudo apt install -y \
  libssl-dev libsqlite3-dev zlib1g-dev libbrotli-dev \
  nlohmann-json3-dev libspdlog-dev libfmt-dev
```

Then check the machine:

```bash
vix doctor
vix info
```

Use `vix doctor` to diagnose tools and environment readiness.

Use `vix info` to inspect Vix paths, registry state, store state, global packages, and artifact cache state.

## Create a production user

A production app should not run as root.

Create a dedicated user:

```bash
sudo useradd --system --create-home --shell /usr/sbin/nologin vix
sudo mkdir -p /home/vix/apps/myapp
sudo chown -R vix:vix /home/vix/apps
```

Clone your project:

```bash
sudo -u vix git clone https://github.com/example/myapp.git /home/vix/apps/myapp
cd /home/vix/apps/myapp
```

## Configure environment

Create `.env`:

```bash
sudo -u vix nano /home/vix/apps/myapp/.env
```

Example:

```dotenv
SERVER_HOST=127.0.0.1
SERVER_PORT=8080
SERVER_TLS_ENABLED=false

VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
VIX_COLOR=never

APP_ENV=production
```

When Nginx handles HTTPS, keep the Vix app local and plain HTTP:

```dotenv
SERVER_TLS_ENABLED=false
```

Nginx terminates TLS and proxies to the local Vix process.

## Validate environment

Before building or deploying:

```bash
vix env check --production
```

To show values safely:

```bash
vix env check --production --show-values
```

Secrets are always masked.

Use this command when:

```txt
.env may be missing
.env.example may be incomplete
required production variables may be missing
systemd environment may not match project expectations
```

## Install dependencies

If your project uses the registry:

```bash
vix registry sync
vix install
```

Use `vix install`, not the old `vix deps` wording.

`vix install` installs exact locked dependencies from `vix.lock`.

## Build a release version

Build the app:

```bash
vix build --preset release
```

Validate it:

```bash
vix check --tests
```

If the app uses SQLite or MySQL support, use the build options supported by your project:

```bash
vix build --preset release --with-sqlite
vix build --preset release --with-mysql
```

The exact flags depend on the project configuration.

## Test locally before service setup

Before creating the service, test the app manually:

```bash
vix run
```

Then check the local health endpoint:

```bash
curl -i http://127.0.0.1:8080/health
```

Stop the app with `Ctrl+C`.

## Create the systemd service

Use:

```bash
vix service init
```

This uses your production config to generate, install, and enable a systemd service.

Then check service state:

```bash
vix service status
```

Restart when needed:

```bash
vix service restart
```

Stop:

```bash
vix service stop
```

Start:

```bash
vix service start
```

The service keeps the app running after SSH disconnects and after server restarts.

## Install the Nginx proxy

Use:

```bash
vix proxy nginx init
```

This generates and installs the Nginx site config.

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

Issue or renew a certificate:

```bash
vix proxy nginx certbot
```

## Plain HTTP setup

For first server testing, TLS can be disabled.

The proxy forwards:

```txt
http://example.com/
  -> http://127.0.0.1:8080/
```

This is useful before enabling HTTPS.

## HTTPS setup

For real production, enable TLS.

The proxy should redirect HTTP to HTTPS:

```txt
http://example.com
  -> https://example.com
```

Then HTTPS forwards to the local app:

```txt
https://example.com
  -> http://127.0.0.1:8080
```

Vix proxy can use certificate paths from your production config.

## WebSocket setup

If the app uses WebSocket, enable the WebSocket proxy section.

Example public URL:

```txt
wss://example.com/ws
```

Local upstream:

```txt
ws://127.0.0.1:9090/ws
```

Check it:

```bash
vix health websocket
vix ws check
```

Use verbose mode when debugging:

```bash
vix ws check --verbose
```

## Health checks

Check everything:

```bash
vix health
```

Check only the local app:

```bash
vix health local
```

Check the public endpoint:

```bash
vix health public
```

Check WebSocket:

```bash
vix health websocket
```

A good health route returns a simple response:

```json
{
  "ok": true,
  "service": "myapp"
}
```

A more detailed production health response can include:

```json
{
  "ok": true,
  "service": "myapp",
  "database": "ok",
  "sync": "enabled"
}
```

## Logs

Read all configured logs:

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

Show the last 100 error lines:

```bash
vix logs errors --lines 100
```

Use logs immediately after a failed deploy:

```bash
vix logs errors --lines 100
```

## Deployment

After the first setup, use:

```bash
vix deploy
```

A deploy can:

```txt
pull latest code
build the app
run tests
restart the systemd service
check local health
check public health
check proxy config
reload proxy
print logs on failure
rollback when enabled
```

Preview the deployment without executing:

```bash
vix deploy --dry-run
```

Run with more details:

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

## Recommended first production setup

Use this flow on a new server:

```bash
cd /home/vix/apps/myapp

vix doctor
vix info

vix registry sync
vix install

vix env check --production
vix build --preset release
vix check --tests

vix service init
vix service status

vix proxy nginx init
vix proxy nginx check

vix health
vix logs
```

This gives you a working production baseline.

## Recommended deployment flow

After the first setup:

```bash
cd /home/vix/apps/myapp

vix env check --production
vix deploy --dry-run
vix deploy --verbose
vix health
```

For normal deploys:

```bash
vix deploy
vix health
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

This usually means the app is running, but Nginx, TLS, DNS, firewall, or public routing needs attention.

### Public HTTP works, WebSocket fails

Run:

```bash
vix health websocket
vix ws check --verbose
vix proxy nginx check
vix logs proxy --lines 100
```

Check the WebSocket path, upstream port, and proxy upgrade headers.

### Service does not start

Run:

```bash
vix service status
vix logs app --lines 100
vix env check --production
```

Common causes:

```txt
missing environment variables
wrong working directory
wrong command
missing binary
port already in use
permission problem
```

### Deployment failed

Run:

```bash
vix logs errors --lines 100
vix service status
vix health
```

Then inspect which deploy step failed.

## Database production notes

For SQLite:

```dotenv
DATABASE_ENGINE=sqlite
DATABASE_DEFAULT_NAME=/home/vix/apps/myapp/data/app.db
```

Check database state:

```bash
vix db status
```

Run migrations:

```bash
vix db migrate
```

Create a backup:

```bash
vix db backup
```

For ORM workflows:

```bash
vix orm status --db myapp --dir ./migrations
vix orm migrate --db myapp --dir ./migrations
```

For MySQL, create a dedicated database user:

```sql
CREATE DATABASE myapp;
CREATE USER 'myapp'@'localhost' IDENTIFIED BY 'change-me';
GRANT SELECT, INSERT, UPDATE, DELETE ON myapp.* TO 'myapp'@'localhost';
FLUSH PRIVILEGES;
```

## Firewall

Allow SSH, HTTP, and HTTPS:

```bash
sudo ufw allow OpenSSH
sudo ufw allow 80
sudo ufw allow 443
sudo ufw enable
```

Do not expose the local app port publicly unless you explicitly need it.

The app should usually listen on:

```txt
127.0.0.1:8080
```

not:

```txt
0.0.0.0:8080
```

when it is behind Nginx.

## Common production errors

### 502 Bad Gateway

Nginx cannot reach the Vix app.

Check:

```bash
vix service status
vix health local
vix logs app --lines 100
```

### 504 Gateway Timeout

The app accepted the connection but did not respond fast enough.

Check:

```bash
vix logs app --lines 100
vix health local
```

Possible causes:

```txt
slow database query
blocked request
overloaded VPS
bad upstream timeout
```

### WebSocket closes immediately

Check:

```bash
vix ws check --verbose
vix proxy nginx check
vix logs proxy --lines 100
```

Possible causes:

```txt
wrong WebSocket path
wrong upstream port
missing upgrade headers
timeout too short
```

### App works locally but not through the domain

Check:

```bash
vix health local
vix health public
vix proxy nginx check
```

Then check DNS, firewall, TLS, and Nginx logs.

## Common mistakes

### Running production manually forever

Manual run:

```bash
vix run
```

Production run:

```bash
vix service init
vix service status
```

Production should be managed by systemd.

### Running the app as root

Use a dedicated user.

Example:

```txt
vix
```

not:

```txt
root
```

### Forgetting the working directory

Relative paths depend on the service working directory.

Examples:

```txt
.env
public/
data/
storage/
```

If the working directory is wrong, the app may start but fail to find files.

### Debug logging forever

Use this for debugging:

```dotenv
VIX_LOG_LEVEL=debug
```

Use this for normal production:

```dotenv
VIX_LOG_LEVEL=info
```

### Exposing internal routes

Protect admin, P2P control, deploy, and internal diagnostic routes.

Routes like this should not be public without authentication:

```txt
POST /p2p/connect
POST /admin/*
POST /internal/*
```

### Deploying without health checks

Always check after deploy:

```bash
vix health
```

## Production checklist

- [ ] Server has required build tools
- [ ] Vix CLI works
- [ ] `vix doctor` passes enough checks
- [ ] Project dependencies are installed with `vix install`
- [ ] `.env` exists
- [ ] `vix env check --production` passes
- [ ] Release build works
- [ ] Tests pass
- [ ] App listens on localhost
- [ ] Health route works locally
- [ ] systemd service starts
- [ ] Logs are visible
- [ ] Nginx proxy is installed
- [ ] `vix proxy nginx check` passes
- [ ] Domain points to server
- [ ] HTTPS works
- [ ] WebSocket works if needed
- [ ] `vix health` passes
- [ ] `vix deploy --dry-run` looks correct
- [ ] `vix deploy` works

## Recommended production structure

```txt
/home/vix/apps/myapp/
├── build-release/
├── data/
├── public/
├── src/
├── .env
├── vix.json
└── vix.lock

/etc/systemd/system/myapp.service
/etc/nginx/sites-available/myapp
/etc/nginx/sites-enabled/myapp
```

## What you should remember

A Vix production app is a normal native Linux service, but Vix gives you commands to manage the full workflow.

```txt
browser
  -> HTTPS
  -> Nginx
  -> Vix app on localhost
  -> systemd
```

First setup:

```bash
vix env check --production
vix build --preset release
vix check --tests
vix service init
vix proxy nginx init
vix health
```

Normal deploy:

```bash
vix deploy
vix health
```

Debugging:

```bash
vix health
vix logs errors --lines 100
vix service status
vix proxy nginx check
```

The core idea is:

```txt
Vix should not only build the app.
Vix should help you run, expose, check, log, and deploy it safely.
```

## Next chapter

[Next: Next steps](/book/17-next-steps)
