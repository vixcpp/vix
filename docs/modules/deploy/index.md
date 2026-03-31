# Deploy (Vix.cpp)

This guide shows a simple, reliable deployment for a Vix HTTP server on Linux using systemd.
It assumes you already built a single binary (or you can build on the server).

## Goal

- Run your Vix server as a service (auto start, auto restart)
- Put Nginx in front (optional but common)
- Add TLS with Certbot (optional)

## Folder layout

Example layout on the server:

```bash
/opt/vix-app/
  bin/
    app            # your compiled binary
  config/
    config.json    # optional
  public/          # optional static files
  logs/            # optional (or use journald only)
```

## 1) Build and copy the binary

### Option A: build locally, copy to server

```bash
scp ./app user@YOUR_SERVER:/opt/vix-app/bin/app
ssh user@YOUR_SERVER "chmod +x /opt/vix-app/bin/app"
```

### Option B: build on the server

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config libboost-all-dev libssl-dev

# Example:
git clone https://github.com/vixcpp/vix.git
cd vix
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp build/app /opt/vix-app/bin/app
chmod +x /opt/vix-app/bin/app
```

## 2) systemd service

Create:

```bash
sudo nano /etc/systemd/system/vix-app.service
```

Paste:

```ini
[Unit]
Description=Vix App
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=www-data
Group=www-data
WorkingDirectory=/opt/vix-app

# Run the app (change port)
ExecStart=/opt/vix-app/bin/app

# If you need args:
# ExecStart=/opt/vix-app/bin/app --port 8080

Restart=always
RestartSec=2
TimeoutStopSec=10

# Minimal hardening (safe defaults)
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/vix-app

# Environment variables (optional)
Environment=VIX_LOG_LEVEL=info
# Or load a file:
# EnvironmentFile=/opt/vix-app/.env

[Install]
WantedBy=multi-user.target
```

Then:

```bash
sudo mkdir -p /opt/vix-app/logs
sudo chown -R www-data:www-data /opt/vix-app

sudo systemctl daemon-reload
sudo systemctl enable --now vix-app
sudo systemctl status vix-app --no-pager
```

### Logs

```bash
journalctl -u vix-app -f
```

## 3) Open the port (firewall)

If you expose the port directly:

```bash
sudo ufw allow 8080/tcp
sudo ufw status
```

Most of the time you put Nginx in front and only expose 80/443.

## 4) Nginx reverse proxy (recommended)

Install Nginx:

```bash
sudo apt install -y nginx
```

Create a site:

```bash
sudo nano /etc/nginx/sites-available/vix-app
```

Example config (HTTP only):

```nginx
server {
  listen 80;
  server_name your-domain.com;

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

Enable and reload:

```bash
sudo ln -sf /etc/nginx/sites-available/vix-app /etc/nginx/sites-enabled/vix-app
sudo nginx -t
sudo systemctl reload nginx
```

## 5) TLS with Certbot (optional)

```bash
sudo apt install -y certbot python3-certbot-nginx
sudo certbot --nginx -d your-domain.com
```

Certbot will update your Nginx config and install auto renewal.

## 6) Deploy update workflow (simple and safe)

When you have a new binary:

```bash
scp ./app user@YOUR_SERVER:/opt/vix-app/bin/app.new
ssh user@YOUR_SERVER '
  set -e
  sudo systemctl stop vix-app
  sudo mv /opt/vix-app/bin/app.new /opt/vix-app/bin/app
  sudo chmod +x /opt/vix-app/bin/app
  sudo systemctl start vix-app
  sudo systemctl status vix-app --no-pager
'
```

If you want less downtime, you can:
- run on a different port, healthcheck it, then switch Nginx upstream
- or use two services (blue/green)

## 7) Health check ideas

Add a route like:

- `GET /health` returns `{ "ok": true }`

Then:

```bash
curl -i http://127.0.0.1:8080/health
```

## Common issues

### Service starts then exits
- Check logs: `journalctl -u vix-app -n 200 --no-pager`
- Ensure the binary is executable and the WorkingDirectory is correct

### Permission errors (static files, uploads)
- Make sure `www-data` owns needed folders
- Keep uploads in `/opt/vix-app` or update `ReadWritePaths`

### Wrong client IP inside the app
- Use `X-Forwarded-For` and `X-Real-IP` from Nginx
- Ensure your app reads forwarded headers if you support that feature

## Minimal checklist

- `systemctl status vix-app`
- `journalctl -u vix-app -f`
- `nginx -t`
- `curl -i http://127.0.0.1:8080/health`


