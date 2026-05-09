# Production deployment

In the previous chapter, you learned P2P.
Now you will learn how to deploy a Vix application in production.

```txt
browser → HTTPS → Nginx → Vix app → systemd
```

## Why production deployment matters

During development: `vix dev`.
In production, your application needs:
- A release build
- A stable working directory
- Environment variables
- Automatic restart
- Logs
- A reverse proxy
- HTTPS
- Health checks
- Predictable ports

## Production architecture

```txt
Internet → Nginx → 127.0.0.1:8080 → Vix app → systemd
```

Nginx handles public HTTP/HTTPS. The Vix app listens locally. systemd keeps the app alive.

## Development vs production

| Development       | Production              |
|-------------------|-------------------------|
| `vix dev`         | Release binary.         |
| Hot reload        | Stable process.         |
| Terminal logs     | Systemd logs.           |
| Local browser     | Nginx reverse proxy.    |
| Debug settings    | Production environment. |
| Manual restart    | Automatic restart.      |

## Prepare the server

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  nginx certbot python3-certbot-nginx \
  libssl-dev libsqlite3-dev zlib1g-dev libbrotli-dev \
  nlohmann-json3-dev libspdlog-dev libfmt-dev
```

## Create a production user

```bash
sudo useradd --system --create-home --shell /usr/sbin/nologin vix
sudo mkdir -p /home/vix/apps/myapp
sudo chown -R vix:vix /home/vix/apps
```

## Configure environment

```bash
sudo -u vix nano /home/vix/apps/myapp/.env
```

```dotenv
SERVER_PORT=8080
SERVER_HOST=127.0.0.1
SERVER_TLS_ENABLED=false

VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
VIX_COLOR=never

# SQLite
DATABASE_ENGINE=sqlite
DATABASE_DEFAULT_NAME=/home/vix/apps/myapp/data/app.db

# MySQL (if used)
DATABASE_ENGINE=mysql
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=myapp
DATABASE_DEFAULT_PASSWORD=change-me
DATABASE_DEFAULT_NAME=myapp
```

> When Nginx handles HTTPS, keep `SERVER_TLS_ENABLED=false` — Nginx terminates TLS and proxies to local Vix.

## Build a release binary

```bash
cd /home/vix/apps/myapp

sudo -u vix vix build --preset release
# With SQLite:
sudo -u vix vix build --preset release --with-sqlite
# With MySQL:
sudo -u vix vix build --preset release --with-mysql
```

## Test the binary manually

```bash
cd /home/vix/apps/myapp
sudo -u vix ./build-release/myapp
```

```bash
# In another terminal
curl -i http://127.0.0.1:8080/health
```

Stop with `Ctrl+C` then proceed to systemd.

## Create a systemd service

```bash
sudo nano /etc/systemd/system/vix-myapp.service
```

```ini
[Unit]
Description=Vix MyApp service
After=network.target

[Service]
Type=simple
User=vix
Group=vix
WorkingDirectory=/home/vix/apps/myapp
ExecStart=/home/vix/apps/myapp/build-release/myapp
Restart=always
RestartSec=3
Environment=VIX_LOG_LEVEL=info
Environment=VIX_LOG_FORMAT=kv
Environment=VIX_COLOR=never
LimitNOFILE=65535

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable vix-myapp
sudo systemctl start vix-myapp
sudo systemctl status vix-myapp
```

## Read logs

```bash
journalctl -u vix-myapp -f         # follow
journalctl -u vix-myapp -n 100     # last 100 lines
journalctl -u vix-myapp -b         # since boot
```

## Nginx reverse proxy

```bash
sudo nano /etc/nginx/sites-available/myapp
```

```nginx
server {
    listen 80;
    server_name example.com www.example.com;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

```bash
sudo ln -s /etc/nginx/sites-available/myapp /etc/nginx/sites-enabled/myapp
sudo nginx -t
sudo systemctl reload nginx
curl -i http://example.com/health
```

## Nginx for WebSocket

```nginx
location / {
    proxy_pass http://127.0.0.1:8080;
    proxy_http_version 1.1;
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
    proxy_read_timeout 3600;
    proxy_send_timeout 3600;
}
```

## Enable HTTPS

```bash
sudo certbot --nginx -d example.com -d www.example.com
sudo certbot renew --dry-run
```

After Certbot, your config adds SSL and a redirect from HTTP to HTTPS automatically.

## Health checks

```cpp
app.get("/health", [](Request &, Response &res)
        {
          res.json({"ok", true, "service", "myapp"});
        });
```

Production health with more detail:

```json
{
  "ok": true,
  "service": "myapp",
  "database": "ok",
  "sync": "enabled"
}
```

## Deployment flow

```bash
cd /home/vix/apps/myapp
sudo -u vix git pull
sudo -u vix vix build --preset release
sudo systemctl restart vix-myapp
curl -i http://127.0.0.1:8080/health
curl -i https://example.com/health
```

## Useful systemd commands

```bash
sudo systemctl start|stop|restart|status vix-myapp
sudo systemctl enable|disable vix-myapp
journalctl -u vix-myapp -f
```

## Useful Nginx commands

```bash
sudo nginx -t
sudo systemctl reload nginx
sudo systemctl status nginx
ls /etc/nginx/sites-enabled
```

## Firewall

```bash
sudo ufw allow OpenSSH
sudo ufw allow 80
sudo ufw allow 443
sudo ufw enable
```

## Database production notes

**SQLite:**

```dotenv
DATABASE_ENGINE=sqlite
DATABASE_DEFAULT_NAME=/home/vix/apps/myapp/data/app.db
```

**MySQL:**

```sql
CREATE DATABASE myapp;
CREATE USER 'myapp'@'localhost' IDENTIFIED BY 'change-me';
GRANT SELECT, INSERT, UPDATE, DELETE ON myapp.* TO 'myapp'@'localhost';
FLUSH PRIVILEGES;
```

## Common production errors

### 502 Bad Gateway

Nginx cannot reach the Vix app.
Check: `sudo systemctl status vix-myapp` and `curl -i http://127.0.0.1:8080/health`.

### 504 Gateway Timeout

App accepted the connection but did not respond fast enough.
Check for slow database queries or overloaded VPS.

### WebSocket closes immediately

Add Nginx upgrade headers and longer timeouts.

### App works locally but not through domain

Check: `sudo nginx -t`, DNS records, firewall, TLS certificate.

## Common mistakes

### Running the app as root

Always use a dedicated user: `User=vix`.

### Forgetting the working directory

```ini
WorkingDirectory=/home/vix/apps/myapp
```

Relative paths (`.env`, `public/`, `data/`) depend on this.

### Using debug logs forever

```dotenv
VIX_LOG_LEVEL=info
```

### Exposing P2P control routes without auth

Routes like `POST /p2p/connect` must be protected or kept internal.

## Deployment checklist

- [ ] Release build works
- [ ] `.env` exists
- [ ] App listens on localhost
- [ ] Health route works locally
- [ ] systemd service starts
- [ ] systemd restarts after failure
- [ ] Logs visible with `journalctl`
- [ ] Nginx config passes `nginx -t`
- [ ] Domain points to server
- [ ] HTTPS enabled
- [ ] Certbot renewal works
- [ ] Database credentials not hardcoded
- [ ] Admin routes protected

## Recommended production structure

```txt
/home/vix/apps/myapp/
├── build-release/
│   └── myapp
├── data/
│   └── app.db
├── public/
├── src/
├── .env
└── vix.json

/etc/systemd/system/vix-myapp.service
/etc/nginx/sites-available/myapp
/etc/nginx/sites-enabled/myapp
```

## What you should remember

A Vix production app is a normal native Linux service.

```txt
browser → HTTPS → Nginx → Vix app on localhost → systemd
```

```bash
vix build --preset release   # build
sudo systemctl start vix-myapp  # run
sudo nginx -t && sudo systemctl reload nginx  # expose
sudo certbot --nginx -d example.com  # HTTPS
```

The core idea: **development uses `vix dev`, production runs a release binary.**

## Next chapter

[Next: Next steps](/book/21-next-steps)
