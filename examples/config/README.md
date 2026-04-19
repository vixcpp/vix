# Config examples

These examples show how Vix reads configuration from `.env` files through `vix::config::Config`.

## Examples

- `01_server_port.cpp` : read `SERVER_PORT`
- `02_database_config.cpp` : read database settings
- `03_logging_config.cpp` : read logging settings
- `04_websocket_config.cpp` : build websocket config from core config
- `05_layered_env.cpp` : load layered env files like `.env.production`

## Quick start

Copy:

```bash
cp examples/config/.env.example .env
```

Then run an example:
```bash
vix run examples/config/01_server_port.cpp
```
