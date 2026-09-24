# Vix HTTPS examples

These examples show how to run a Vix HTTP application with built-in HTTPS enabled from `.env`.

Built-in HTTPS is useful for local development, internal tools and simple self-hosted deployments.

For production deployments, Vix is optimized to run behind a TLS-terminating reverse proxy such as Nginx, Caddy or Traefik.

## Create local certificates

```bash
mkdir -p certs

openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout certs/local.key \
  -out certs/local.crt \
  -days 365 \
  -subj "/CN=localhost"
```
## Example .env

```env
SERVER_PORT=8443
SERVER_TLS_ENABLED=true
SERVER_TLS_CERT_FILE=certs/local.crt
SERVER_TLS_KEY_FILE=certs/local.key
SERVER_BENCH_MODE=false
SERVER_REQUEST_TIMEOUT=2000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=20
```

Run
```bash
vix run examples/tls/01_https_basic.cpp
```

Then test:

```bash
curl -k https://127.0.0.1:8443/
curl -k https://127.0.0.1:8443/bench
```

Benchmark

```bash
wrk -t8 -c100 -d30s --latency https://127.0.0.1:8443/bench
```

The built-in TLS transport is intended for simple HTTPS usage. For high-performance production deployments, terminate TLS with Nginx, Caddy, Traefik or another reverse proxy and forward local HTTP traffic to Vix.
EOF
