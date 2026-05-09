# Production: Nginx + systemd

This guide shows how to deploy a Vix application in production behind Nginx and systemd.

## Final architecture

```txt
browser → HTTPS → Nginx → reverse proxy → Vix app → systemd service
```

## Production goal

```txt
Vix listens on localhost:8080
Nginx handles public HTTP/HTTPS
systemd keeps the app running
Certbot manages TLS certificates
```

## Build the app

```bash
cd /home/vix/apps/myapp
vix build --preset release
# With SQLite:   vix build --preset release --with-sqlite
# With MySQL:    vix build --preset release --with-mysql
```

## Test locally before systemd

```bash
./build-release/myapp
curl -i http://127.0.0.1:8080/health
```

## Use .env

```dotenv
SERVER_PORT=8080
SERVER_TLS_ENABLED=false
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
VIX_COLOR=never
```

> When Nginx handles HTTPS, your Vix app runs plain HTTP locally.

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

## Check logs

```bash
journalctl -u vix-myapp -f
journalctl -u vix-myapp -n 100
```

## Restart after deployment

```bash
sudo systemctl restart vix-myapp
curl -i http://127.0.0.1:8080/health
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
```

## Nginx for WebSocket

```nginx
proxy_set_header Upgrade $http_upgrade;
proxy_set_header Connection "upgrade";
```

## Enable HTTPS with Certbot

```bash
sudo apt install certbot python3-certbot-nginx
sudo certbot --nginx -d example.com -d www.example.com
sudo certbot renew --dry-run
```

## HTTPS Nginx config shape

```nginx
server {
    listen 443 ssl;
    server_name example.com www.example.com;
    ssl_certificate /etc/letsencrypt/live/example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/example.com/privkey.pem;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}

server {
    listen 80;
    server_name example.com www.example.com;
    return 301 https://$host$request_uri;
}
```

## Firewall

```bash
sudo ufw allow OpenSSH
sudo ufw allow 'Nginx Full'
sudo ufw enable
```

## Useful commands

```bash
# systemd
sudo systemctl start|stop|restart|status vix-myapp
sudo systemctl enable|disable vix-myapp
journalctl -u vix-myapp -f

# Nginx
sudo nginx -t
sudo systemctl reload nginx
sudo systemctl status nginx
```

## Deployment flow

```bash
cd /home/vix/apps/myapp
git pull origin main
vix build --preset release
sudo systemctl restart vix-myapp
curl -i http://127.0.0.1:8080/health
curl -i https://example.com/health
```

## Common production errors

### 502 Bad Gateway

App is not running.
Check: `sudo systemctl status vix-myapp` and `curl -i http://127.0.0.1:8080/health`.

### 504 Gateway Timeout

Slow database query or overloaded VPS. Check logs.

### WebSocket closes immediately

Add Nginx upgrade headers and longer timeouts:

```nginx
proxy_read_timeout 3600;
proxy_send_timeout 3600;
proxy_set_header Upgrade $http_upgrade;
proxy_set_header Connection "upgrade";
```

## Security checklist

- HTTPS enabled
- Vix app not exposed directly to public internet
- Nginx forwards real client headers
- `.env` is not public and has safe permissions (`chmod 600`)
- Service runs as non-root user
- Firewall allows only SSH and Nginx
- Login routes have rate limiting
- CORS allows only trusted origins

## Deployment checklist

- Build succeeds locally
- Binary runs manually on server
- `/health` works locally
- systemd service starts
- Nginx config passes `nginx -t`
- Domain points to server
- HTTPS certificate is installed
- `/health` works through public domain
- Logs are visible
- Restart flow is documented

## What to use next

- [REST API guide](/guides/build-rest-api)
- [SQLite API guide](/guides/sqlite-api)
- [WebSocket chat guide](/guides/websocket-chat)
