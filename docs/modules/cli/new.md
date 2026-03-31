# vix new

Create a new Vix project.

---

## Usage

```bash
vix new <name|path> [options]
```

---

## Options

```
-d, --dir <base_dir>    Base directory where the project folder will be created
--app                   Generate an application template
--lib                   Generate a library template (header-only)
--force                 Overwrite if directory exists (no prompt)
```

---

## Environment Variables

```
VIX_NONINTERACTIVE=1    Disable interactive prompts
CI=1                    Disable interactive prompts
```

---

# Interactive Mode

When no template flag is provided, Vix will prompt:

## Template

- Application
- Library (header-only)

## Features (Application)

Optional:

- ORM (database layer)
- Sanitizers (debug only)
- Static C++ runtime

## Advanced

- Full static binary (maps to VIX_LINK_FULL_STATIC=ON)

---

# Examples

Create an application:

```bash
vix new api
```

Create a library:

```bash
vix new tree --lib
```

Create inside a directory:

```bash
vix new blog -d ./projects
```

Force overwrite:

```bash
vix new api --force
```

---

# Output Example (Application)

```
✔ Project created.
Location: /home/user/api

Next steps:
  cd api/
  vix build
  vix run
```

Tip:

```
vix dev
```

---

# Output Example (Library)

```
✔ Project created.
Location: /home/user/test_lib

Next steps:
  cd test_lib/
  vix tests
```

---

# Behavior Notes

- If directory exists, you will be prompted unless `--force` is used.
- Library template is header-only by default.
- Application template includes CMake configuration.
- Interactive mode is disabled in CI environments.

---

This command bootstraps the full Vix project structure.

