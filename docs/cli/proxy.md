# vix proxy

`vix proxy` manages reverse proxy configuration for a Vix application.

Use it when you want to expose a Vix backend through Nginx, configure a domain, enable WebSocket proxying, validate proxy rules, reload Nginx, or issue a Let's Encrypt certificate.

```bash
vix proxy nginx init
```

## Overview

`vix proxy` is the production reverse proxy command for Vix.

It helps connect a local Vix application to a public domain.

It can:

- generate an Nginx site config
- install the config under `/etc/nginx/sites-available`
- enable the site under `/etc/nginx/sites-enabled`
- validate Nginx configuration
- reload Nginx safely
- configure HTTP reverse proxying
- configure WebSocket reverse proxying
- configure HTTPS redirects
- validate TLS certificate paths
- check TLS certificate quality when crypto support is available
- issue or renew Let's Encrypt certificates with Certbot
- detect missing proxy headers
- detect wrong upstream ports
- detect missing WebSocket upgrade headers
- detect missing proxy timeouts

This command is part of the Vix production workflow.

The goal is to avoid manually writing and debugging Nginx configs for every Vix backend.

## Platform support

`vix proxy` is currently supported on:

```txt
Linux
```

The Nginx provider uses:

```txt
nginx
systemctl
sudo
certbot
/etc/nginx/sites-available
/etc/nginx/sites-enabled
/etc/letsencrypt/live
```

On unsupported platforms, Vix reports that `vix proxy` is currently supported on Linux only.

## Usage

```bash
vix proxy <provider> <command>
```

Currently supported provider:

```txt
nginx
```

## Commands

```bash
vix proxy nginx <command>
```

| Command   | Purpose                                                                  |
| --------- | ------------------------------------------------------------------------ |
| `init`    | Generate, install, enable, validate, and reload an Nginx config.         |
| `check`   | Validate the installed Nginx proxy config.                               |
| `reload`  | Validate the proxy config, then reload Nginx.                            |
| `certbot` | Issue or renew a Let's Encrypt certificate and install final TLS config. |

## Basic examples

```bash
# Generate and install Nginx config
vix proxy nginx init

# Validate installed Nginx proxy config
vix proxy nginx check

# Validate and reload Nginx
vix proxy nginx reload

# Issue or renew a Let's Encrypt certificate
vix proxy nginx certbot
```

## Typical production workflow

A common production setup looks like this:

```bash
vix build --preset release
vix service install
vix service start
vix proxy nginx init
vix proxy nginx check
vix doctor production
```

If you want HTTPS with Let's Encrypt:

```bash
vix proxy nginx certbot
vix proxy nginx check
vix doctor production
```

After changing proxy configuration:

```bash
vix proxy nginx init
vix proxy nginx reload
vix doctor production
```

## Configuration source

`vix proxy nginx` reads configuration from:

```txt
vix.json
```

under:

```txt
production.proxy
```

Example:

```json
{
  "name": "PulseGrid",
  "production": {
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

## Proxy config fields

| Field                                  | Purpose                                        |
| -------------------------------------- | ---------------------------------------------- |
| `production.proxy.domain`              | Public domain for the app.                     |
| `production.proxy.http.port`           | Local HTTP port used by the Vix app.           |
| `production.proxy.websocket.enabled`   | Enables WebSocket proxy support.               |
| `production.proxy.websocket.path`      | Public WebSocket path. Default shape is `/ws`. |
| `production.proxy.websocket.port`      | Local WebSocket port.                          |
| `production.proxy.tls.enabled`         | Enables TLS config generation.                 |
| `production.proxy.tls.certificate`     | TLS certificate path.                          |
| `production.proxy.tls.certificate_key` | TLS private key path.                          |

## Project name detection

Vix detects the app name from:

```txt
vix.json name
*.vix file name
current directory name
```

The app name is used for Nginx site files.

For an app named:

```txt
PulseGrid
```

Vix writes:

```txt
/etc/nginx/sites-available/PulseGrid
/etc/nginx/sites-enabled/PulseGrid
```

## Domain

The domain is required.

Example:

```json
{
  "production": {
    "proxy": {
      "domain": "pulsegrid.example.com"
    }
  }
}
```

If the domain is missing, Vix prints:

```txt
Missing proxy domain.
Fix: add production.proxy.domain to vix.json
```

## HTTP proxy

The HTTP proxy forwards public HTTP traffic to the local Vix app.

Example config:

```json
{
  "production": {
    "proxy": {
      "domain": "pulsegrid.example.com",
      "http": {
        "port": 8080
      }
    }
  }
}
```

This creates an upstream like:

```nginx
proxy_pass http://127.0.0.1:8080;
```

The generated config includes important proxy headers:

```nginx
proxy_set_header Connection "";
proxy_set_header Host $host;
proxy_set_header X-Real-IP $remote_addr;
proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
proxy_set_header X-Forwarded-Proto $scheme;
```

It also includes HTTP proxy timeouts:

```nginx
proxy_connect_timeout 10s;
proxy_send_timeout 60s;
proxy_read_timeout 60s;
```

## WebSocket proxy

Enable WebSocket proxying when your app exposes a WebSocket endpoint.

Example:

```json
{
  "production": {
    "proxy": {
      "domain": "pulsegrid.example.com",
      "http": {
        "port": 8080
      },
      "websocket": {
        "enabled": true,
        "path": "/ws",
        "port": 9090
      }
    }
  }
}
```

This generates a WebSocket location like:

```nginx
location = /ws {
    proxy_pass http://127.0.0.1:9090/;
    proxy_http_version 1.1;

    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";

    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;

    proxy_read_timeout 3600s;
    proxy_send_timeout 3600s;
    proxy_buffering off;
}
```

## WebSocket path normalization

If the WebSocket path does not start with `/`, Vix normalizes it.

Example:

```json
{
  "production": {
    "proxy": {
      "websocket": {
        "enabled": true,
        "path": "ws",
        "port": 9090
      }
    }
  }
}
```

Vix treats it as:

```txt
/ws
```

If no path is provided, the default shape is:

```txt
/ws
```

## TLS proxy

Enable TLS when you want HTTPS.

Example:

```json
{
  "production": {
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

When TLS is enabled and a domain is configured, Vix uses Let's Encrypt style default paths:

```txt
/etc/letsencrypt/live/<domain>/fullchain.pem
/etc/letsencrypt/live/<domain>/privkey.pem
```

For example:

```txt
/etc/letsencrypt/live/pulsegrid.example.com/fullchain.pem
/etc/letsencrypt/live/pulsegrid.example.com/privkey.pem
```

You can also set explicit certificate paths:

```json
{
  "production": {
    "proxy": {
      "domain": "pulsegrid.example.com",
      "tls": {
        "enabled": true,
        "certificate": "/etc/letsencrypt/live/pulsegrid.example.com/fullchain.pem",
        "certificate_key": "/etc/letsencrypt/live/pulsegrid.example.com/privkey.pem"
      }
    }
  }
}
```

## Generated TLS behavior

When TLS is enabled, Vix generates two server blocks:

1. HTTP server on port `80`
2. HTTPS server on port `443`

The HTTP block redirects to HTTPS:

```nginx
return 301 https://$host$request_uri;
```

The HTTPS block serves the app through the configured upstream.

## Plain HTTP proxy

If TLS is disabled, Vix generates a plain HTTP config.

Example:

```json
{
  "production": {
    "proxy": {
      "domain": "pulsegrid.example.com",
      "http": {
        "port": 8080
      },
      "tls": {
        "enabled": false
      }
    }
  }
}
```

This generates only a port `80` server block.

## Initialize Nginx proxy

Run:

```bash
vix proxy nginx init
```

This command:

1. loads `production.proxy` from `vix.json`
2. prints a proxy summary
3. renders an Nginx config
4. writes the config to a temporary file
5. copies it to `/etc/nginx/sites-available/<app>`
6. enables it with a symlink under `/etc/nginx/sites-enabled/<app>`
7. runs `sudo nginx -t`
8. reloads Nginx
9. prints success when the proxy is installed

You may be asked for `sudo` permission.

## Init summary

Example output shape:

```txt
Proxy Init
App: PulseGrid
Domain: pulsegrid.example.com
HTTP: 127.0.0.1:8080
WebSocket: 127.0.0.1:9090 /ws
TLS: enabled
Site file: /etc/nginx/sites-available/PulseGrid
Enabled path: /etc/nginx/sites-enabled/PulseGrid
Certificate: /etc/letsencrypt/live/pulsegrid.example.com/fullchain.pem
Certificate key: /etc/letsencrypt/live/pulsegrid.example.com/privkey.pem
```

## Validate proxy config

Run:

```bash
vix proxy nginx check
```

This validates the installed proxy config.

It checks:

- domain exists
- site file exists
- site is enabled
- `server_name` matches
- HTTP upstream port matches
- forwarded headers exist
- HTTP proxy timeouts exist
- WebSocket location exists when enabled
- WebSocket upstream port matches when enabled
- WebSocket upgrade headers exist when enabled
- WebSocket timeouts exist when enabled
- TLS certificate paths match when TLS is enabled
- HTTPS redirect exists when TLS is enabled
- Nginx is installed
- `sudo nginx -t` passes
- TLS certificate looks valid when crypto support is available

## Check summary

Example output shape:

```txt
Proxy Check
App: PulseGrid
Domain: pulsegrid.example.com
HTTP upstream: 127.0.0.1:8080
WS upstream: 127.0.0.1:9090 /ws
TLS: enabled
Site file: /etc/nginx/sites-available/PulseGrid
Enabled: yes
```

If everything is correct:

```txt
nginx config is valid
proxy config looks good
```

## Reload Nginx safely

Run:

```bash
vix proxy nginx reload
```

This command runs the proxy check first.

If the check fails, reload is aborted.

```txt
Proxy check failed. Nginx reload aborted.
Fix: run `vix proxy nginx check`
```

If the check passes, Vix reloads Nginx:

```bash
sudo systemctl reload nginx
```

If Nginx is not active, Vix suggests starting it:

```bash
sudo systemctl start nginx
```

## Issue or renew a Let's Encrypt certificate

Run:

```bash
vix proxy nginx certbot
```

This command helps bootstrap HTTPS.

It performs this flow:

1. checks that `production.proxy.domain` exists
2. checks that `nginx` is available
3. checks that `certbot` is available
4. installs a temporary HTTP config
5. validates Nginx config
6. reloads or starts Nginx
7. runs `sudo certbot --nginx -d <domain>`
8. checks the generated certificate files
9. installs the final Vix TLS proxy config
10. validates Nginx config again
11. reloads or starts Nginx again

This is useful because Certbot often needs a working HTTP config before issuing the first certificate.

## Certbot requirements

`vix proxy nginx certbot` requires:

```txt
nginx
certbot
```

If Certbot is missing, Vix suggests:

```bash
sudo apt install certbot python3-certbot-nginx
```

## Certbot command

Vix runs a command shaped like:

```bash
sudo certbot --nginx -d pulsegrid.example.com
```

After Certbot succeeds, Vix expects:

```txt
/etc/letsencrypt/live/pulsegrid.example.com/fullchain.pem
/etc/letsencrypt/live/pulsegrid.example.com/privkey.pem
```

Then it installs the final TLS config.

## TLS certificate checks

When Vix is built with crypto support, `vix proxy nginx check` can inspect the TLS certificate.

It can detect:

- invalid PEM X.509 certificate
- expired certificate
- certificate domain mismatch

If crypto support is not available in the current Vix CLI build, Vix reports:

```txt
TLS certificate checks are not available in this build.
Fix: rebuild the Vix CLI with the crypto module enabled
```

## Full HTTP example

```json
{
  "name": "PulseGrid",
  "production": {
    "proxy": {
      "domain": "pulsegrid.example.com",
      "http": {
        "port": 8080
      },
      "tls": {
        "enabled": false
      }
    }
  }
}
```

Then run:

```bash
vix proxy nginx init
vix proxy nginx check
```

## Full HTTPS example

```json
{
  "name": "PulseGrid",
  "production": {
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

Then run:

```bash
vix proxy nginx certbot
vix proxy nginx check
```

## Full HTTPS + WebSocket example

```json
{
  "name": "PulseGrid",
  "production": {
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

Then run:

```bash
vix proxy nginx certbot
vix proxy nginx check
vix doctor production
```

## Generated config shape

With HTTPS and WebSocket enabled, Vix generates a config shaped like this:

```nginx
server {
    listen 80;
    listen [::]:80;
    server_name pulsegrid.example.com;

    return 301 https://$host$request_uri;
}

server {
    listen 443 ssl http2;
    listen [::]:443 ssl http2;
    server_name pulsegrid.example.com;

    ssl_certificate /etc/letsencrypt/live/pulsegrid.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/pulsegrid.example.com/privkey.pem;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_http_version 1.1;

        proxy_set_header Connection "";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        proxy_connect_timeout 10s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }

    location = /ws {
        proxy_pass http://127.0.0.1:9090/;
        proxy_http_version 1.1;

        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";

        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        proxy_read_timeout 3600s;
        proxy_send_timeout 3600s;
        proxy_buffering off;
    }
}
```

## Relationship with `vix service`

`vix service` manages the app process.

`vix proxy nginx` manages the public reverse proxy.

A complete backend production setup usually needs both:

```bash
vix build --preset release
vix service install
vix service start
vix proxy nginx certbot
vix doctor production
```

## Relationship with `vix doctor production`

`vix doctor production` inspects the production state.

After configuring the proxy, run:

```bash
vix doctor production
```

It can detect:

- Nginx proxy configured
- proxy target
- public URL
- TLS status
- local health
- public health
- readiness score

## Relationship with `vix health`

`vix proxy nginx` validates proxy configuration.

`vix health` validates runtime availability.

After proxy setup, use:

```bash
vix health local
vix health public
```

Then:

```bash
vix doctor production
```

## Recommended production setup

For a public HTTP app:

```bash
vix build --preset release
vix service install
vix service start
vix proxy nginx init
vix proxy nginx check
vix doctor production
```

For a public HTTPS app:

```bash
vix build --preset release
vix service install
vix service start
vix proxy nginx certbot
vix proxy nginx check
vix doctor production
```

For a public HTTPS app with WebSocket:

```bash
vix build --preset release
vix service install
vix service start
vix proxy nginx certbot
vix proxy nginx check
vix health public
vix doctor production
```

## Commands reference

| Command                   | Description                                                            |
| ------------------------- | ---------------------------------------------------------------------- |
| `vix proxy nginx init`    | Generate, install, enable, test, and reload Nginx config.              |
| `vix proxy nginx check`   | Validate the installed Nginx proxy config.                             |
| `vix proxy nginx reload`  | Validate config and reload Nginx.                                      |
| `vix proxy nginx certbot` | Issue or renew Let's Encrypt certificate and install final TLS config. |
| `vix proxy nginx --help`  | Show Nginx proxy help.                                                 |
| `vix proxy --help`        | Show proxy help.                                                       |

## Environment and system paths

| Path or tool                                   | Purpose                                      |
| ---------------------------------------------- | -------------------------------------------- |
| `/etc/nginx/sites-available/<app>`             | Installed Nginx site config.                 |
| `/etc/nginx/sites-enabled/<app>`               | Enabled Nginx site symlink.                  |
| `/etc/letsencrypt/live/<domain>/fullchain.pem` | Default Let's Encrypt certificate.           |
| `/etc/letsencrypt/live/<domain>/privkey.pem`   | Default Let's Encrypt private key.           |
| `nginx`                                        | Nginx executable.                            |
| `systemctl`                                    | Reloads or starts Nginx.                     |
| `certbot`                                      | Issues or renews Let's Encrypt certificates. |

## Common workflows

### Create a plain HTTP proxy

```bash
vix proxy nginx init
vix proxy nginx check
```

### Create HTTPS proxy with Certbot

```bash
vix proxy nginx certbot
vix proxy nginx check
```

### Reinstall proxy after changing `vix.json`

```bash
vix proxy nginx init
vix proxy nginx check
```

### Validate before reload

```bash
vix proxy nginx check
vix proxy nginx reload
```

### Check production after proxy setup

```bash
vix doctor production
```

## Common mistakes

### Missing proxy domain

Wrong:

```json
{
  "production": {
    "proxy": {
      "http": {
        "port": 8080
      }
    }
  }
}
```

Correct:

```json
{
  "production": {
    "proxy": {
      "domain": "pulsegrid.example.com",
      "http": {
        "port": 8080
      }
    }
  }
}
```

### Using the wrong HTTP upstream port

If your app listens on port `8080`, configure:

```json
{
  "production": {
    "proxy": {
      "http": {
        "port": 8080
      }
    }
  }
}
```

Then run:

```bash
vix proxy nginx init
vix proxy nginx check
```

### Forgetting WebSocket upgrade headers

Do not edit the generated Nginx config manually unless needed.

If `vix proxy nginx check` says WebSocket upgrade headers are missing, regenerate:

```bash
vix proxy nginx init
```

### Enabling TLS before certificates exist

If TLS is enabled and certificate files do not exist yet, use:

```bash
vix proxy nginx certbot
```

This bootstraps HTTP first, runs Certbot, then installs the final TLS config.

### Running `reload` before `check`

`reload` already runs `check`.

If check fails, reload is aborted.

Use:

```bash
vix proxy nginx check
```

to see the exact issue.

### Expecting proxy to start the app

`vix proxy nginx` manages Nginx.

It does not start your Vix app.

Start the app with:

```bash
vix service start
```

or run it manually with:

```bash
vix run
```

### Expecting proxy to build the app

`vix proxy nginx` does not build the app.

Build first:

```bash
vix build --preset release
```

Then configure service and proxy.

## Troubleshooting

### Nginx is not installed

Install Nginx:

```bash
sudo apt install nginx
```

Then run:

```bash
vix proxy nginx init
```

### Certbot is not installed

Install Certbot:

```bash
sudo apt install certbot python3-certbot-nginx
```

Then run:

```bash
vix proxy nginx certbot
```

### Nginx config is invalid

Run:

```bash
sudo nginx -t
```

Then regenerate:

```bash
vix proxy nginx init
```

### Nginx reload fails

Check whether Nginx is active:

```bash
systemctl is-active nginx
```

Start it:

```bash
sudo systemctl start nginx
```

Or reload it:

```bash
sudo systemctl reload nginx
```

Then run:

```bash
vix proxy nginx check
```

### Site file is missing

If check reports:

```txt
Nginx site file not found
```

run:

```bash
vix proxy nginx init
```

### Site is not enabled

If check reports that the site is not enabled, run:

```bash
vix proxy nginx init
```

Or manually create the symlink shown by Vix.

### TLS certificate does not match domain

Check:

```json
{
  "production": {
    "proxy": {
      "domain": "pulsegrid.example.com"
    }
  }
}
```

Then issue or renew:

```bash
vix proxy nginx certbot
```

### TLS certificate is expired

Renew:

```bash
vix proxy nginx certbot
```

Then check:

```bash
vix proxy nginx check
```

### Public URL does not work

Check the full stack:

```bash
vix service status
vix service health
vix proxy nginx check
vix health public
vix doctor production
```

Common causes:

- app service is not running
- wrong HTTP port
- wrong WebSocket port
- Nginx config not enabled
- TLS certificate missing
- DNS does not point to the server
- firewall blocks ports `80` or `443`

## Best practices

Keep proxy configuration in `vix.json`.

Use `vix proxy nginx init` instead of editing Nginx config manually.

Use `vix proxy nginx check` before debugging public health.

Use `vix proxy nginx reload` instead of reloading Nginx manually.

Use `vix proxy nginx certbot` for first-time HTTPS setup.

Use WebSocket config only when your app actually exposes a WebSocket endpoint.

Run `vix doctor production` after proxy changes.

Run `vix health public` after TLS changes.

Do not ignore warnings from `vix proxy nginx check`.

## Related commands

| Command                 | Purpose                                   |
| ----------------------- | ----------------------------------------- |
| `vix service`           | Manage the systemd app service.           |
| `vix doctor production` | Inspect production readiness.             |
| `vix health`            | Check local, public, or WebSocket health. |
| `vix logs`              | Inspect app and proxy logs.               |
| `vix deploy`            | Deploy a production app.                  |
| `vix build`             | Build the app binary.                     |
| `vix run`               | Run the app manually.                     |

## Next step

Check production readiness.

[Open the production doctor guide](/cli/doctor)
