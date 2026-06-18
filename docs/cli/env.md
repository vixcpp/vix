# vix env

`vix env` checks project environment files.

Use it when you want to verify `.env`, compare it with `.env.example`, validate required production variables, or detect differences between local env values and systemd service environment.

```bash
vix env check
```

## Overview

`vix env` is the environment validation command for Vix projects.

It can check:

- whether `.env` exists
- whether `.env.example` exists
- which variables are loaded from `.env`
- which variables are documented in `.env.example`
- variables missing from `.env`
- variables missing from `.env.example`
- required production variables
- systemd service environment variables
- differences between `.env` and systemd environment
- secret variables that must be masked in output

It is useful during:

- local development setup
- onboarding a new project
- production deployment
- service debugging
- systemd environment validation
- checking whether `.env.example` documents the real project requirements

## Usage

```bash
vix env <command> [options]
```

## Commands

| Command | Purpose                          |
| ------- | -------------------------------- |
| `check` | Check project environment files. |

## Basic examples

```bash
# Check local .env and .env.example
vix env check

# Check production requirements too
vix env check --production

# Show values with secrets masked
vix env check --show-values

# Show production values with non-secrets visible
vix env check --production --show-values --no-mask

# Force masked output
vix env check --production --show-values --masked
```

## Configuration sources

`vix env check` reads:

```txt
.env
.env.example
vix.json
systemd service environment when --production is used
```

Production configuration is read from:

```txt
production.env.required
production.service.name
production.deploy.service
```

## Local check

Run:

```bash
vix env check
```

This checks `.env` and `.env.example` in the current project directory.

Example output shape:

```txt
Env Check
App: PulseGrid
Project: /home/user/PulseGrid
.env: found
.env.example: found
Production: no
Masked: yes
Show values: no

Variables
✔ APP_ENV : loaded
✔ SERVER_PORT : loaded
⚠ DATABASE_URL : missing from .env
⚠ INTERNAL_ONLY : missing from .env.example
```

## What local check validates

`vix env check` compares variables from `.env` and `.env.example`.

It reports:

| Case                        | Meaning                                                             |
| --------------------------- | ------------------------------------------------------------------- |
| loaded                      | Variable exists in `.env`.                                          |
| missing from `.env`         | Variable exists in `.env.example`, but not in `.env`.               |
| missing from `.env.example` | Variable exists in `.env`, but is not documented in `.env.example`. |
| missing                     | Variable was known from another source, but no value was found.     |

The goal is to keep local configuration and documentation aligned.

## Production check

Run:

```bash
vix env check --production
```

Production mode adds extra checks.

It reads required production variables from:

```txt
production.env.required
```

It also reads the systemd service environment when a service is configured.

Example:

```json
{
  "name": "PulseGrid",
  "production": {
    "env": {
      "required": ["APP_ENV", "SERVER_PORT", "DATABASE_URL", "JWT_SECRET"]
    },
    "service": {
      "name": "pulsegrid"
    }
  }
}
```

Then run:

```bash
vix env check --production
```

## Production summary

Example output shape:

```txt
Env Check
App: PulseGrid
Project: /home/user/PulseGrid
.env: found
.env.example: found
Production: yes
Masked: yes
Show values: no
Service: pulsegrid
Required vars: 4
Systemd vars: 4
```

## Required production variables

Required production variables are configured in `vix.json`.

Example:

```json
{
  "production": {
    "env": {
      "required": ["APP_ENV", "SERVER_PORT", "DATABASE_URL", "JWT_SECRET"]
    }
  }
}
```

If a required variable is missing from both `.env` and systemd, Vix reports it as an error:

```txt
Missing required production env
✖ JWT_SECRET
Fix: set missing keys in .env or systemd Environment
```

## Service name detection

In production mode, Vix tries to find the systemd service name from:

```txt
production.service.name
production.deploy.service
```

Example with `production.service.name`:

```json
{
  "production": {
    "service": {
      "name": "pulsegrid"
    }
  }
}
```

Example with `production.deploy.service`:

```json
{
  "production": {
    "deploy": {
      "service": "pulsegrid"
    }
  }
}
```

If a service name is found, Vix reads systemd environment with:

```bash
systemctl show <service> -p Environment --value
```

## Systemd environment check

When `--production` is enabled and a service name is configured, Vix compares `.env` with the systemd service environment.

If the same variable exists in both places but has a different value, Vix reports it:

```txt
Systemd env differs
⚠ SERVER_PORT
Fix: update the systemd service environment, then run `sudo systemctl daemon-reload` and restart the service
```

This prevents a common production bug:

```txt
.env says one thing
systemd runs with another value
```

## `.env` file format

Vix supports common `.env` syntax.

Example:

```dotenv
APP_ENV=development
SERVER_PORT=8080
DATABASE_URL=sqlite://storage/app.db
JWT_SECRET=change-me
```

It also supports `export`:

```dotenv
export SERVER_PORT=8080
```

Quoted values are supported:

```dotenv
APP_NAME="PulseGrid"
JWT_SECRET='secret-value'
```

Inline comments are supported when the `#` starts after whitespace and is not inside quotes:

```dotenv
SERVER_PORT=8080 # local server port
APP_NAME="PulseGrid # backend"
```

Blank lines and full-line comments are ignored:

```dotenv
# local configuration
SERVER_PORT=8080
```

## `.env.example`

`.env.example` should document every variable needed by the project.

Example:

```dotenv
APP_ENV=development
SERVER_PORT=8080
DATABASE_URL=sqlite://storage/app.db
JWT_SECRET=
```

Commit `.env.example`.

Do not commit `.env` if it contains local values or secrets.

## Secret detection

Vix detects secret variables by name.

A variable is treated as secret when its name contains:

```txt
secret
password
passwd
token
api_key
apikey
private_key
credential
```

Examples:

```txt
JWT_SECRET
DATABASE_PASSWORD
API_TOKEN
PRIVATE_KEY
SERVICE_CREDENTIAL
```

Secrets are always masked when values are printed.

## Show values

By default, `vix env check` does not print values.

Use:

```bash
vix env check --show-values
```

Example output shape:

```txt
✔ SERVER_PORT : loaded = ********
✔ JWT_SECRET : loaded = ******** [secret]
```

By default, values are masked.

## Masked output

Use `--masked` to force masked output:

```bash
vix env check --show-values --masked
```

This prints values as:

```txt
********
```

Secrets are always masked, even with `--no-mask`.

## Unmasked non-secret values

Use `--no-mask` with `--show-values` to show non-secret values:

```bash
vix env check --show-values --no-mask
```

Example:

```txt
✔ SERVER_PORT : loaded = 8080
✔ APP_ENV : loaded = development
✔ JWT_SECRET : loaded = ******** [secret]
```

Secrets remain masked.

## Value display rules

| Option                    | Behavior                                              |
| ------------------------- | ----------------------------------------------------- |
| no `--show-values`        | Values are not printed.                               |
| `--show-values`           | Values are printed, masked by default.                |
| `--show-values --masked`  | All printed values are masked.                        |
| `--show-values --no-mask` | Non-secret values are visible. Secrets remain masked. |

## Variable status

Each variable can show one or more tags.

| Tag          | Meaning                                          |
| ------------ | ------------------------------------------------ |
| `[secret]`   | Variable name looks sensitive.                   |
| `[required]` | Variable is listed in `production.env.required`. |
| `[systemd]`  | Variable was found in systemd Environment.       |

Example:

```txt
✖ JWT_SECRET : required missing [secret] [required]
✔ SERVER_PORT : loaded [required] [systemd]
```

## Result behavior

`vix env check` fails when:

- no environment variables are found
- required production variables are missing
- variables from `.env.example` are missing from `.env`
- systemd environment differs from `.env` in production mode

It warns when:

- `.env` is missing
- `.env.example` is missing
- variables exist in `.env` but are not documented in `.env.example`

## Exit codes

| Exit code | Meaning                                                    |
| --------- | ---------------------------------------------------------- |
| `0`       | Environment check passed.                                  |
| `1`       | Environment check failed or command arguments are invalid. |

This makes `vix env check` useful in CI and deployment scripts.

## Full local example

```json
{
  "name": "PulseGrid"
}
```

`.env.example`:

```dotenv
APP_ENV=development
SERVER_PORT=8080
DATABASE_URL=sqlite://storage/PulseGrid.db
JWT_SECRET=
```

`.env`:

```dotenv
APP_ENV=development
SERVER_PORT=8080
DATABASE_URL=sqlite://storage/PulseGrid.db
JWT_SECRET=local-secret
```

Run:

```bash
vix env check
```

Expected result:

```txt
environment check passed
```

## Full production example

`vix.json`:

```json
{
  "name": "PulseGrid",
  "production": {
    "env": {
      "required": ["APP_ENV", "SERVER_PORT", "DATABASE_URL", "JWT_SECRET"]
    },
    "service": {
      "name": "pulsegrid"
    }
  }
}
```

`.env.example`:

```dotenv
APP_ENV=production
SERVER_PORT=8080
DATABASE_URL=sqlite://storage/PulseGrid.db
JWT_SECRET=
```

Run:

```bash
vix env check --production
```

This checks:

```txt
.env
.env.example
production.env.required
systemd Environment from pulsegrid service
```

## Relationship with `vix service`

`vix service` installs and runs the systemd service.

`vix env check --production` verifies whether required environment variables are present and whether systemd has different values.

A good workflow is:

```bash
vix env check --production
vix service install
vix service restart
vix env check --production
```

If environment values differ, update the service and restart:

```bash
sudo systemctl daemon-reload
vix service restart
```

## Relationship with `vix doctor production`

`vix doctor production` gives a broader production readiness view.

`vix env check --production` focuses specifically on environment configuration.

Use both:

```bash
vix env check --production
vix doctor production
```

## Relationship with `vix run` and `vix dev`

`vix run` and `vix dev` use environment configuration when running apps.

Use `vix env check` before debugging runtime issues caused by missing variables.

Example:

```bash
vix env check
vix dev
```

## Relationship with `vix deploy`

A production deploy should validate environment before restarting the app.

Example:

```bash
vix env check --production
vix deploy
vix health
```

If required variables are missing, fix them before deployment.

## Options

| Option          | Description                                               |
| --------------- | --------------------------------------------------------- |
| `check`         | Check project environment files.                          |
| `--production`  | Validate production env requirements and systemd env.     |
| `--show-values` | Print env values. Secrets are always masked.              |
| `--masked`      | Mask printed values.                                      |
| `--no-mask`     | Do not mask non-secret values when using `--show-values`. |
| `-h, --help`    | Show help.                                                |

## Commands reference

| Command                                 | Description                                          |
| --------------------------------------- | ---------------------------------------------------- |
| `vix env check`                         | Check `.env` and `.env.example`.                     |
| `vix env check --production`            | Check production required variables and systemd env. |
| `vix env check --show-values`           | Print values with masking.                           |
| `vix env check --show-values --no-mask` | Print non-secret values.                             |
| `vix env --help`                        | Show help.                                           |

## Common workflows

### Check local env files

```bash
vix env check
```

### Check production env

```bash
vix env check --production
```

### Show values safely

```bash
vix env check --show-values
```

### Show non-secret values

```bash
vix env check --show-values --no-mask
```

### Validate before deployment

```bash
vix env check --production
vix db status
vix build --preset release
vix service restart
vix health
```

### Debug production env mismatch

```bash
vix env check --production --show-values
vix service status
vix logs app --errors
```

## Common mistakes

### Running without a command

Wrong:

```bash
vix env
```

Correct:

```bash
vix env check
```

### Expecting secrets to be printed

Secrets are always masked.

Even with:

```bash
vix env check --show-values --no-mask
```

secret-like keys are printed as:

```txt
********
```

### Forgetting `.env.example`

Wrong:

```txt
.env exists
.env.example missing
```

Correct:

```bash
cp .env .env.example
```

Then remove real secrets from `.env.example`.

### Adding variables to `.env` but not documenting them

If Vix reports:

```txt
Missing from .env.example
```

add the variable to `.env.example`.

### Adding variables to `.env.example` but not `.env`

If Vix reports:

```txt
Missing from .env
```

copy the key into `.env`.

### Using production mode without required keys

Production mode is more useful when `vix.json` defines:

```json
{
  "production": {
    "env": {
      "required": ["APP_ENV", "SERVER_PORT"]
    }
  }
}
```

### Forgetting to update systemd environment

If Vix reports:

```txt
Systemd env differs
```

update the service environment, reload systemd, and restart the service.

```bash
sudo systemctl daemon-reload
vix service restart
```

## Troubleshooting

### `.env` not found

Create it from `.env.example`:

```bash
cp .env.example .env
```

Then run:

```bash
vix env check
```

### `.env.example` not found

Create it:

```bash
touch .env.example
```

Add documented keys:

```dotenv
APP_ENV=development
SERVER_PORT=8080
```

Then run:

```bash
vix env check
```

### No environment variables found

Add variables to `.env` or `.env.example`.

Example:

```dotenv
APP_ENV=development
SERVER_PORT=8080
```

### Required production variable missing

Add it to `.env` or systemd Environment.

Example:

```dotenv
JWT_SECRET=change-me
```

Then run:

```bash
vix env check --production
```

### Systemd env differs from `.env`

Inspect the service:

```bash
systemctl show pulsegrid -p Environment --value
```

Update the service environment, reload systemd, and restart:

```bash
sudo systemctl daemon-reload
vix service restart
```

Then check again:

```bash
vix env check --production
```

### Service name is missing in production mode

Add one of these to `vix.json`.

Preferred:

```json
{
  "production": {
    "service": {
      "name": "pulsegrid"
    }
  }
}
```

Alternative:

```json
{
  "production": {
    "deploy": {
      "service": "pulsegrid"
    }
  }
}
```

Then run:

```bash
vix env check --production
```

## Best practices

Keep `.env.example` committed.

Do not commit real `.env` secrets.

Document every project variable in `.env.example`.

Use `production.env.required` for production-critical variables.

Run `vix env check` before `vix dev`.

Run `vix env check --production` before deployment.

Use `--show-values --no-mask` only when you are sure the terminal output is safe.

Trust secret masking for keys containing `secret`, `password`, `passwd`, `token`, `api_key`, `apikey`, `private_key`, or `credential`.

Run `vix service restart` after changing systemd environment.

## Related commands

| Command                 | Purpose                                        |
| ----------------------- | ---------------------------------------------- |
| `vix run`               | Run an app with environment configuration.     |
| `vix dev`               | Start development mode with env-aware runtime. |
| `vix service`           | Manage production systemd service.             |
| `vix doctor production` | Inspect production readiness.                  |
| `vix health`            | Check endpoint health after env changes.       |
| `vix logs`              | Inspect runtime errors caused by missing env.  |
| `vix deploy`            | Deploy production app.                         |

## Next step

Check database configuration.

[Open the database guide](/cli/db)
