# Cache System

Vix uses cache to make repeated work faster.

```txt
first run  → resolve / build / store
next run   → reuse cached state when safe
```

Goal: **do not repeat expensive work when the inputs did not change.**

## Big picture

```txt
~/.vix/
├── registry/index/     → local package metadata
├── store/git/          → downloaded package repositories/commits
├── global/             → globally installed packages
│   └── installed.json
└── cache/build/        → build/compiled artifacts
```

## Main cache areas

| Area            | Path                    | Purpose                                 |
|-----------------|-------------------------|-----------------------------------------|
| Registry index  | `~/.vix/registry/index` | Stores package metadata.                |
| Store cache     | `~/.vix/store/git`      | Stores downloaded packages and commits. |
| Global packages | `~/.vix/global`         | Stores globally installed packages.     |
| Artifact cache  | `~/.vix/cache/build`    | Stores build artifacts.                 |

## vix info

```bash
vix info
```

Example output:

```txt
Info
• version    : v2.5.2
• root       : /home/user/.vix
Environment
• registry   : /home/user/.vix/registry/index [present]
• store      : /home/user/.vix/store/git [present]
• global root: /home/user/.vix/global [present]
• global manifest: /home/user/.vix/global/installed.json [present]
Caches
• artifact cache : /home/user/.vix/cache/build [present]
• store packages : 43
• store commits  : 128
• global packages: 11
• store size     : 41.43 MB
• artifact size  : 1.61 KB
Status
• registry usable : yes
• store usable    : yes
• artifacts usable: yes
• globals usable  : yes
✔ Vix environment detected
```

**present** = path exists. **usable** = Vix believes it can use that area.

## Artifact cache (direct compile)

```txt
main.cpp + flags + feature flags + Vix version
  ↓
cache key
  ↓
cached binary (reused if inputs unchanged)
```

Without cache: compile on every run. With cache: reuse when safe.

Feature flags create different cache entries:

```bash
vix run main.cpp                  # different cache entry
vix run main.cpp --with-sqlite    # different cache entry
vix run main.cpp --san            # different cache entry
```

## Store cache

The store caches downloaded package sources by commit:

```txt
download once → store locally → reuse when needed
```

Store commits are stable (immutable content). Registry metadata may need refreshing.

## Cache safety

**Cache must never break correctness.** Reuse only when:

- same source file content
- same compiler flags
- same dependency versions
- same build mode
- same compiler-compatible environment

If unsure, rebuild.

## Cache and offline development

If dependencies are already in the store cache, Vix may not need network for those dependencies — useful when working offline.

## Troubleshooting checklist

```bash
vix info    # check all cache areas

# If artifacts look broken
rm -rf ~/.vix/cache/build
vix run main.cpp  # rebuilds

# If store looks corrupted
rm -rf ~/.vix/store/git  # warning: re-downloads dependencies

# Check permissions
ls -la ~/.vix
```

## Common mistakes

### Using sudo and breaking permissions

```bash
# Wrong — root-owned files block later user commands
sudo vix run main.cpp
sudo vix install

# Correct — run as your normal user
vix run main.cpp
```

### Confusing app cache and Vix cache

```cpp
// Application HTTP cache — in your app
res.header("Cache-Control", "public, max-age=60");
```

```txt
Vix internal cache — ~/.vix/cache/build
```

These solve different problems.

### Deleting the whole Vix root too quickly

```bash
# Too aggressive — removes registry, store, globals, artifacts
rm -rf ~/.vix

# Better — inspect first
vix info
# Then remove only what you understand
rm -rf ~/.vix/cache/build   # just artifacts
```

### Treating cache as source of truth

Cache accelerates repeated work. Source code, manifests, lock files, and package repositories remain the real source of truth.

## Design principles

1. **Invisible when healthy:** developers should not think about cache during normal work.
2. **Visible when debugging:** `vix info` exposes cache state clearly.
3. **Safe:** never reuse cached data when inputs do not match.
4. **Rebuildable:** artifact cache can always be regenerated.
5. **Correctness over speed:** a fast but wrong cache is worse than no cache.

## What you should remember

```txt
~/.vix/registry/index  → package metadata
~/.vix/store/git       → downloaded packages and commits
~/.vix/global          → global packages
~/.vix/cache/build     → build artifacts
```

Use `vix info` to inspect cache state.
Use `vix --version` and the failing command for bug reports.

The core idea: **cache makes Vix faster, but correctness always comes first.**

Next: [Performance](/internals/performance)
