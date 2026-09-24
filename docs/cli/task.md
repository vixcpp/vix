# vix task

`vix task` runs reusable project tasks.

Use it when you want to define common workflows once in `vix.json` and run them by name.

## Usage

```bash
vix task <name> [args...]
vix task --list
vix task --help
```

## What it does

`vix task` is a small task runner built into the Vix CLI. It can run built-in project tasks, custom tasks from `vix.json`, task dependencies, multiple commands in order, platform-specific commands, commands with variables, commands with environment variables, and commands from a specific working directory.

## Basic usage

```bash
vix task dev
vix task check
vix task --list
vix task --help
```

## Built-in tasks

| Task | Command |
|------|---------|
| `dev` | Run in development mode with hot reload |
| `run` | Run the application |
| `fmt` | Format code |
| `check` | Validate build, tests, runtime, and sanitizers |
| `build` | Build project |
| `test` | Run tests with CTest |

## Custom tasks

Custom tasks are defined in `vix.json`:

```json
{
  "tasks": {
    "fmt": "vix fmt",
    "ci": ["vix fmt --check", "vix check --tests"]
  }
}
```

## Task formats

A task can be defined as a string, array, or object.

### String task

```json
{
  "tasks": {
    "fmt": "vix fmt"
  }
}
```

### Array task

```json
{
  "tasks": {
    "ci": [
      "vix fmt --check",
      "vix check --tests"
    ]
  }
}
```

If one command fails, the task stops instead of continuing.

### Object task

```json
{
  "tasks": {
    "release": {
      "description": "Release pipeline",
      "deps": ["fmt", "test"],
      "vars": {
        "preset": "release"
      },
      "env": {
        "VIX_LOG_LEVEL": "info"
      },
      "cwd": "${project_dir}",
      "commands": [
        "vix build --preset ${preset}",
        "vix check --preset ${preset} --tests"
      ]
    }
  }
}
```

## Task dependencies

```json
{
  "tasks": {
    "fmt": "vix fmt --check",
    "test": "vix tests",
    "ci": {
      "deps": ["fmt", "test"],
      "commands": ["vix check --tests"]
    }
  }
}
```

Execution order: `fmt` → `test` → `ci`

## Variables

Global variables are defined in the root `vars` object:

```json
{
  "vars": {
    "preset": "dev-ninja"
  },
  "tasks": {
    "build": "vix build --preset ${preset}"
  }
}
```

Task variables override global variables:

```json
{
  "vars": {
    "preset": "dev-ninja"
  },
  "tasks": {
    "release": {
      "vars": {
        "preset": "release"
      },
      "command": "vix build --preset ${preset}"
    }
  }
}
```

## Built-in variables

| Variable | Meaning |
|----------|---------|
| `${task}` | Current task |
| `${task_name}` | Current task name |
| `${platform}` | Current platform |
| `${project_dir}` | Project directory |
| `${root}` | Project root |

## Environment variables

```json
{
  "tasks": {
    "debug": {
      "env": {
        "VIX_LOG_LEVEL": "debug"
      },
      "command": "vix run"
    }
  }
}
```

## Working directory

```json
{
  "tasks": {
    "build-api": {
      "cwd": "${project_dir}/apps/api",
      "command": "vix build"
    }
  }
}
```

## Platform-specific tasks

Supported platform override keys: `linux`, `windows`, `macos`.

```json
{
  "tasks": {
    "build": {
      "linux": {
        "command": "vix build --preset release"
      },
      "windows": {
        "command": "vix build --preset dev-msvc"
      },
      "macos": {
        "command": "vix build --preset release"
      }
    }
  }
}
```

## Passing arguments to tasks

```bash
vix task release -- --verbose
```

CLI arguments are appended only to the final command of the selected task. Dependencies run normally.

## Custom tasks take priority

If a custom task has the same name as a built-in task, the custom task wins.

## Non-interactive mode

```bash
VIX_NONINTERACTIVE=1 vix task ci
CI=1 vix task ci
```

## Full example

```json
{
  "vars": {
    "preset": "dev-ninja"
  },
  "tasks": {
    "fmt": "vix fmt --check",
    "test": "vix tests",
    "dev": "vix dev",
    "ci": [
      "vix fmt --check",
      "vix check --tests"
    ],
    "release": {
      "description": "Release pipeline",
      "deps": ["fmt", "test"],
      "vars": {
        "preset": "release"
      },
      "env": {
        "VIX_LOG_LEVEL": "info"
      },
      "cwd": "${project_dir}",
      "commands": [
        "vix build --preset ${preset}",
        "vix check --preset ${preset} --tests",
        "vix pack --name api --version 1.0.0"
      ],
      "linux": {
        "command": "vix build --preset ${preset}"
      },
      "windows": {
        "command": "vix build --preset dev-msvc"
      }
    }
  }
}
```

## Recommended project tasks

```json
{
  "tasks": {
    "dev": "vix dev",
    "run": "vix run",
    "fmt": "vix fmt",
    "check": "vix check --tests",
    "test": "vix tests",
    "build": "vix build",
    "ci": [
      "vix fmt --check",
      "vix check --tests"
    ],
    "release": [
      "vix build --preset release",
      "vix check --preset release --tests"
    ]
  }
}
```

## Common workflows

```bash
vix task dev
vix task ci
vix task release
vix task --list
vix task release -- --verbose
```

## Common mistakes

### Forgetting to define tasks under `tasks`

Wrong:

```json
{
  "ci": "vix check --tests"
}
```

Correct:

```json
{
  "tasks": {
    "ci": "vix check --tests"
  }
}
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix dev` | Run development server |
| `vix run` | Build and run app |
| `vix build` | Build project |
| `vix check` | Validate project |
| `vix tests` | Run tests |
| `vix fmt` | Format source files |
| `vix pack` | Package project |

## Next step

Continue with all CLI commands.

[Open the all commands guide](/cli/commands)
