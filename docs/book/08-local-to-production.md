# From Local to Production

A Vix application should move from local development to production without becoming a different project.

Local development should be simple:

```bash id="e3x7s1"
vix dev
```

Production should be repeatable:

```bash id="p1z49q"
vix deploy
```

The app stays the same.

The environment changes.

The workflow becomes stricter.

## The model

The path from local to production is:

```txt id="g14d8x"
local app
  -> local build
  -> local checks
  -> production config
  -> service
  -> proxy
  -> health checks
  -> logs
  -> deploy
```

A serious app should not depend on random manual steps.

The production path must be written down in config and commands.

## Local is for feedback

Local development is about speed.

The normal local loop:

```bash id="a0zdn1"
vix dev
```

This gives:

```txt id="ukz20i"
watch files
rebuild when needed
restart app
show runtime output
```

For one manual run:

```bash id="dyw2sp"
vix run
```

For one file:

```bash id="fg0k9w"
vix run main.cpp
```

Local work should be fast, but not careless.

You still need checks before production.

## Production is for repeatability

Production is not a terminal left open with:

```bash id="x8kc2s"
vix run
```

Production should be controlled by:

```txt id="nxbvp4"
systemd service
Nginx proxy
environment file
health checks
logs
deploy workflow
```

Vix provides commands for these pieces:

```bash id="w4l71a"
vix service
vix proxy nginx
vix health
vix logs
vix deploy
```

The goal is not to hide production.

The goal is to make it repeatable.

## The production contract

A production-ready Vix app should answer:

```txt id="m4301d"
How is it built?
How is it started?
Which user runs it?
Which env file is used?
Which port does it listen on?
Which domain exposes it?
How is HTTPS configured?
How is health checked?
How are logs read?
What happens when deploy fails?
```

If the project cannot answer these questions, production is not ready.

## Build before production

Before production, build clearly.

```bash id="ser17b"
vix build --preset release
```

Then validate:

```bash id="nf87vy"
vix check --tests
```

A good release flow:

```bash id="m60rmu"
vix fmt --check
vix build --preset release
vix check --tests
```

Do not deploy a project that cannot pass its own checks.

## Dependencies before production

After cloning on a server:

```bash id="squx87"
vix install
```

This installs exact locked dependencies from `vix.lock`.

Do not use update for deployment setup.

```txt id="jo00kb"
vix install = reproduce locked dependency state
vix update = change dependency state
```

Production should be reproducible.

That starts with the lockfile.

## Environment files

Local apps often use:

```txt id="k1t75n"
.env
.env.example
```

Production apps should also define required variables:

```txt id="g6xmqn"
production.env.required
```

Example `.env.example`:

```dotenv id="txj4e2"
APP_ENV=development

SERVER_HOST=127.0.0.1
SERVER_PORT=8080
SERVER_TLS_ENABLED=false

VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv

DATABASE_ENGINE=sqlite
DATABASE_DEFAULT_NAME=./data/app.db

JWT_SECRET=change-me
SESSION_SECRET=change-me
```

Example `production.env.required`:

```txt id="d8m9o9"
APP_ENV
SERVER_HOST
SERVER_PORT
VIX_LOG_LEVEL
DATABASE_ENGINE
JWT_SECRET
SESSION_SECRET
```

Check local env:

```bash id="qrt1n1"
vix env check
```

Check production env:

```bash id="xey6km"
vix env check --production
```

Show values only when needed:

```bash id="umslry"
vix env check --production --show-values
```

Secrets are always masked.

## Production config in `vix.json`

Production should be configured in `vix.json`.

The structure can include:

```txt id="u2hw62"
production.service
production.proxy
production.health
production.logs
production.deploy
```

Example:

```json id="uv6f3q"
{
  "production": {
    "service": {
      "name": "api",
      "user": "vix",
      "working_dir": "/home/vix/apps/api",
      "command": "vix run",
      "env_file": "/home/vix/apps/api/.env"
    },
    "proxy": {
      "domain": "api.example.com",
      "http_port": 8080,
      "websocket_enabled": true,
      "websocket_port": 9090,
      "websocket_path": "/ws",
      "tls": {
        "enabled": true,
        "certificate": "/etc/letsencrypt/live/api.example.com/fullchain.pem",
        "certificate_key": "/etc/letsencrypt/live/api.example.com/privkey.pem"
      }
    },
    "health": {
      "service": "api",
      "local": "http://127.0.0.1:8080/health",
      "public": "https://api.example.com/health",
      "websocket": "wss://api.example.com/ws"
    },
    "logs": {
      "service": "api",
      "nginx_access": "/var/log/nginx/api.access.log",
      "nginx_error": "/var/log/nginx/api.error.log"
    }
  }
}
```

The values must match the server.

Do not put fake production config in a real deploy.

## Service layer

The service layer runs the app as a system service.

Use:

```bash id="x934qk"
vix service init
```

Then:

```bash id="skw9z1"
vix service status
vix service restart
vix service stop
```

The service should know:

```txt id="lh4dbk"
service name
working directory
command
user
environment file
restart policy
```

The app should not require someone to start it manually after reboot.

## Why systemd matters

On Linux servers, systemd gives:

```txt id="tjz3da"
startup on boot
restart on failure
status inspection
journal logs
controlled stop and restart
```

A backend app should run as a managed service.

That gives a clean operational model:

```txt id="zh4p5e"
code changes through deploy
runtime through service
logs through journal
public traffic through proxy
```

## Proxy layer

The proxy layer exposes the app publicly.

For Nginx:

```bash id="nfpmjs"
vix proxy nginx init
```

Check config:

```bash id="wq9yah"
vix proxy nginx check
```

Reload:

```bash id="t6t9na"
vix proxy nginx reload
```

Issue or renew certificate:

```bash id="qzvfps"
vix proxy nginx certbot
```

The proxy should map public traffic to the local app.

Example:

```txt id="l40wg3"
https://api.example.com
  -> http://127.0.0.1:8080
```

For WebSocket:

```txt id="z71xnb"
wss://api.example.com/ws
  -> ws://127.0.0.1:9090
```

## TLS model

Production traffic should use HTTPS.

The TLS model:

```txt id="fzupac"
port 80 redirects to 443
port 443 uses certificate and key
Nginx forwards traffic to local app
```

The app can stay local:

```txt id="fj0b3u"
127.0.0.1:8080
```

Nginx handles public TLS.

This keeps the app simple and the public boundary clear.

## WebSocket proxying

If the app uses WebSocket, production config must say it.

Important values:

```txt id="b87b19"
websocket enabled
public WebSocket URL
local WebSocket host
local WebSocket port
WebSocket path
timeout
heartbeat
```

Check WebSocket:

```bash id="ekfs6f"
vix ws check
```

Or through health:

```bash id="cznc6s"
vix health websocket
```

WebSocket should not be assumed healthy just because HTTP works.

## Health checks

Health checks prove the app is usable.

Run all checks:

```bash id="dnknuw"
vix health
```

Local endpoint:

```bash id="xh7j06"
vix health local
```

Public endpoint:

```bash id="u9ae0d"
vix health public
```

WebSocket endpoint:

```bash id="b0drqr"
vix health websocket
```

A health check should report:

```txt id="rnzrun"
target
URL
expected status
actual status
response time
max response time
healthy yes or no
error when present
```

Production without health checks is blind.

## The health route

A backend should expose:

```txt id="p2zu0u"
GET /health
```

Example response:

```json id="e2evlr"
{
  "ok": true,
  "service": "api",
  "status": "healthy"
}
```

This route should be fast.

It should not depend on heavy work.

If it checks database connectivity, make that intentional.

## Logs

Production logs must be easy to read.

Use:

```bash id="jvj9g7"
vix logs
```

Application logs:

```bash id="md0bco"
vix logs app
```

Proxy logs:

```bash id="zfe85x"
vix logs proxy
```

Error logs:

```bash id="w1cju8"
vix logs errors
```

Follow logs:

```bash id="k03m3n"
vix logs app --follow
```

Read recent logs:

```bash id="b57zau"
vix logs errors --lines 100
```

Logs should help answer:

```txt id="ym3yq1"
Did the service start?
Did it crash?
Did Nginx route traffic?
Did health checks fail?
Which error happened first?
```

## Deploy workflow

Deployment should be a sequence of clear steps.

Run:

```bash id="kqq63e"
vix deploy
```

Preview:

```bash id="qmdgh2"
vix deploy --dry-run
```

Verbose:

```bash id="sl4i2v"
vix deploy --verbose
```

Skip pull:

```bash id="huwl1y"
vix deploy --no-pull
```

Skip tests:

```bash id="t3kmxl"
vix deploy --no-tests
```

A deployment can include:

```txt id="it57f9"
git pull
build
tests
service restart
local health check
public health check
proxy check
proxy reload
logs on failure
rollback
```

This is the production workflow in one command.

## Deploy config

Example:

```json id="qy9k72"
{
  "production": {
    "deploy": {
      "pull": true,
      "branch": "main",
      "build": "vix build --preset release",
      "tests": true,
      "test_command": "vix check --tests",
      "service": "api",
      "health_local": true,
      "health_public": true,
      "proxy_check": true,
      "proxy_reload": true,
      "logs_on_failure": true,
      "log_lines": 120,
      "rollback": true
    }
  }
}
```

Each step should be visible.

Deployment should not feel like a black box.

## Dry run first

Before trusting production deploy:

```bash id="yeq5aq"
vix deploy --dry-run
```

A dry run should show the planned steps without executing them.

Use it when:

```txt id="z6jroa"
setting up a new server
changing service config
changing proxy config
changing deploy config
debugging production flow
```

Dry run is not just for beginners.

It prevents avoidable mistakes.

## Rollback

Rollback should exist because deploys can fail.

A deploy can fail because:

```txt id="h5mbwi"
build failed
tests failed
service restart failed
health check failed
proxy config failed
public endpoint failed
```

If rollback is enabled, Vix should attempt to restore the previous working state.

The model:

```txt id="wkuym0"
deploy new version
  -> check health
  -> if failed, restore previous state
```

Rollback should be explicit in config.

## Logs on failure

When deployment fails, the next step is logs.

Config:

```json id="anfmcv"
{
  "production": {
    "deploy": {
      "logs_on_failure": true,
      "log_lines": 120
    }
  }
}
```

This lets deploy show useful context immediately.

A failed deploy without logs forces manual guessing.

## Local checks before deploy

Before deployment:

```bash id="gm9wup"
vix fmt --check
vix build --preset release
vix check --tests
vix env check --production
```

Then:

```bash id="xb5mdz"
vix deploy --dry-run
vix deploy
```

This sequence is simple and serious.

## Server setup order

For a fresh server, use this order:

```bash id="f0n5ht"
vix doctor
vix install
vix env check --production
vix build --preset release
vix service init
vix proxy nginx init
vix health
vix deploy --dry-run
vix deploy
```

This order prevents confusion.

Environment before service.

Service before proxy health.

Proxy before public health.

Deploy after the pieces are ready.

## `vix doctor` in production

Use:

```bash id="k4fswz"
vix doctor
```

With online check:

```bash id="ed7lsb"
vix doctor --online
```

Doctor checks the environment.

Use it when:

```txt id="bp9h18"
tools are missing
Vix behaves strangely
a new server was prepared
upgrade may be needed
build or run fails unexpectedly
```

`vix doctor` diagnoses.

It does not fix everything automatically.

## `vix info` in production

Use:

```bash id="p64agb"
vix info
```

This shows:

```txt id="fk7vvl"
Vix version
Vix root
registry path
store path
global packages
artifact cache
disk usage
local state
```

Use it before cleanup.

Use it when dependency state is unclear.

Use it when registry or store paths matter.

## Registry in production

If production needs project dependencies:

```bash id="vkmksb"
vix registry sync
vix install
```

If the registry is already synced but outdated:

```bash id="e9bynp"
vix registry sync
```

If install fails because a package is not found:

```bash id="gw65zc"
vix registry sync
vix install
```

The registry index is metadata.

The store is package content.

The project lockfile chooses exact versions.

## Store cleanup in production

Check store path:

```bash id="qgdng2"
vix store path
```

Preview cleanup:

```bash id="m8z81g"
vix store gc --project --dry-run
```

Run cleanup:

```bash id="r75xl1"
vix store gc --project
```

Be careful.

Project-scoped GC can remove cached packages not referenced by the current project lockfile.

Always preview first.

## Production database

For SQLite apps:

```bash id="t5dkpt"
vix db status
vix db migrate
vix db backup
```

Before dangerous deploys:

```bash id="ks4p69"
vix db backup
```

For migration workflows:

```bash id="k626zj"
vix orm status --db api --dir ./migrations
vix orm migrate --db api --dir ./migrations
```

Database state is production state.

Treat it carefully.

## Backups

A production app that writes data needs backups.

For SQLite:

```bash id="xq2ed3"
vix db backup
```

This can copy:

```txt id="r824bp"
database file
WAL file when present
SHM file when present
```

Backups are not optional for real systems.

If the app stores important data, backup must be part of the operational model.

## Static files

If the app serves static files, the production model should define where they live.

Example:

```txt id="rdq1q4"
public/
assets/
uploads/
```

Generated build artifacts should not be mixed with user uploads.

A simple separation:

```txt id="z3n845"
public = versioned static files
uploads = runtime user files
dist = packaging output
build = build output
```

This prevents accidental deletion during cleanup.

## Runtime arguments in production

Local runtime arguments can be passed with:

```bash id="x0guj6"
vix run -- --port 8080
```

In production, prefer config and env files.

The service should define the command clearly.

Example:

```txt id="n9viev"
vix run -- --port 8080
```

But do not hide important values in random shell history.

Put stable production values in the service config or environment file.

## Production user

Do not run production apps as root unless there is a real reason.

A production service should define:

```txt id="owz40r"
user = vix
working directory = /home/vix/apps/api
```

The service user should own only what it needs.

This limits damage when something goes wrong.

## Ports

A simple production model:

```txt id="q8j7hf"
app HTTP port: 8080
app WebSocket port: 9090
Nginx public HTTP: 80
Nginx public HTTPS: 443
```

The app listens locally.

Nginx exposes public traffic.

This keeps the public surface small.

## Firewall model

The public server should expose only what is needed.

Usually:

```txt id="aycrto"
22 for SSH
80 for HTTP redirect
443 for HTTPS
```

Application ports like `8080` and `9090` should normally stay local.

This depends on your deployment setup, but the default should be conservative.

## Production output

Production commands should print useful summaries.

For deploy:

```txt id="cpxbd5"
App
Branch
Pull yes or no
Build command
Tests enabled or disabled
Service name
Health checks
Proxy check
Logs on failure
Rollback
Dry run
Verbose
```

For proxy init:

```txt id="i6gl4k"
App
Domain
HTTP upstream
WebSocket upstream
TLS enabled or disabled
Site file
Enabled path
Certificate paths
```

For health:

```txt id="z9cw22"
Target
URL
Expected status
Actual status
Time
Healthy yes or no
```

This is how production stays explainable.

## Local to production example

A full backend flow:

```bash id="shsrtk"
vix new api --template backend
cd api

vix install
vix dev
```

Before commit:

```bash id="n3oojr"
vix fmt --check
vix build
vix check --tests
```

Before release:

```bash id="kq2vdc"
vix build --preset release
vix check --tests
```

On server:

```bash id="j8c8rz"
vix doctor
vix install
vix env check --production
vix build --preset release
vix service init
vix proxy nginx init
vix health
```

Deploy:

```bash id="rh7ou6"
vix deploy --dry-run
vix deploy
```

Inspect if something fails:

```bash id="gutq7t"
vix logs errors --lines 120
vix service status
vix health
```

## Common mistakes

### Running production manually

Wrong:

```bash id="jh6lxn"
ssh server
vix run
```

Correct:

```bash id="w46zal"
vix service init
vix service restart
vix health
```

### Updating dependencies during deploy setup

Wrong:

```bash id="lp98g9"
vix update
```

Correct:

```bash id="mxzzt9"
vix install
```

### Skipping env checks

Wrong:

```bash id="zpz303"
vix deploy
```

Better:

```bash id="dyjy0m"
vix env check --production
vix deploy
```

### Checking only local health

Wrong:

```bash id="br243b"
vix health local
```

Better:

```bash id="z8hryz"
vix health local
vix health public
```

If WebSocket is used:

```bash id="mwmhmx"
vix health websocket
```

### Reloading proxy without checking config

Wrong:

```bash id="p4oyk7"
sudo systemctl reload nginx
```

Better:

```bash id="pob0q6"
vix proxy nginx check
vix proxy nginx reload
```

### Cleaning store without preview

Wrong:

```bash id="ncoeh8"
vix store gc --project
```

Correct:

```bash id="x80xy6"
vix store gc --project --dry-run
vix store gc --project
```

## Production checklist

Before first deploy:

```txt id="o2wnuy"
Vix installed
vix doctor passes
dependencies installed with vix install
.env exists on server
production.env.required is satisfied
release build passes
tests pass
systemd service exists
Nginx config exists
TLS certificate exists if TLS is enabled
local health passes
public health passes
logs are readable
deploy dry run looks correct
```

After deploy:

```txt id="xmr3d6"
service is active
local health is healthy
public health is healthy
WebSocket health is healthy when enabled
logs do not show startup errors
rollback path is known
```

## What you should remember

Local development is for fast feedback.

Production is for repeatable operation.

The model:

```txt id="r6f19h"
vix dev
  -> vix build
  -> vix check
  -> vix service
  -> vix proxy
  -> vix health
  -> vix logs
  -> vix deploy
```

Do not treat production as a different universe.

Use the same project.

Make the environment explicit.

Make the service repeatable.

Make the proxy visible.

Make health checks mandatory.

Make logs easy to read.

The core rule:

```txt id="szwnxs"
local should be fast
production should be repeatable
both should be understandable
```

## Next chapter

[Next: Next Steps](/book/09-next-steps)
