# vix agent

`vix agent` runs the Vix AI agent from the CLI.

Use it when you want to ask a local model a question, analyze a project workspace, or scan a workspace before deeper analysis.

```bash
vix agent ask "Explain Vix.cpp in simple words"
```

## Overview

`vix agent` provides AI-assisted development workflows for Vix.cpp.

It supports three commands:

```txt
vix agent ask
vix agent analyze
vix agent scan
```

The agent can:

```txt
answer questions
analyze a local project
scan workspace files
read project files when allowed
use a local model provider such as Ollama
cache responses
persist run history and memory
optionally run safe local commands
```

The default provider comes from the environment or falls back to Ollama.

## Usage

```bash
vix agent ask <prompt> [options]
vix agent analyze [workspace] [prompt] [options]
vix agent scan [workspace] [options]
```

## Basic examples

```bash
vix agent ask "Explain Vix.cpp in simple words"

vix agent ask "Explain Vix.cpp" --timeout 120000

vix agent ask "Explain this code" \
  --model qwen2.5-coder:1.5b \
  --timeout 120000

vix agent analyze .

vix agent scan .

vix agent ask "Run vix tests if useful" --allow-process
```

## Subcommands

| Command             | Purpose                                        |
| ------------------- | ---------------------------------------------- |
| `vix agent ask`     | Ask the agent a direct question.               |
| `vix agent analyze` | Analyze a workspace and explain the project.   |
| `vix agent scan`    | Scan workspace files and print a file summary. |

## `vix agent ask`

Use `ask` for direct prompts.

```bash
vix agent ask "Explain Vix.cpp in simple words"
```

The prompt is required.

Everything after `ask` becomes the input prompt unless it is an option.

Example:

```bash
vix agent ask "What does this project do?"
```

With workspace:

```bash
vix agent ask "Explain this repository" --workspace .
```

With model and timeout:

```bash
vix agent ask "Explain this code" \
  --model qwen2.5-coder:1.5b \
  --timeout 120000
```

## `vix agent analyze`

Use `analyze` when you want the agent to inspect a local project.

```bash
vix agent analyze .
```

If no prompt is given, Vix uses this default analysis prompt:

```txt
Analyze this project and explain the most important parts.
```

You can also pass a custom prompt after the workspace:

```bash
vix agent analyze . "Explain the build system and main modules"
```

Or use `--workspace`:

```bash
vix agent analyze --workspace . "Explain this project architecture"
```

## Analyze mode behavior

Analyze mode gives the agent project-analysis context.

It tells the agent to focus on:

```txt
local C++ project architecture
modules
folders
build system
CLI commands
runtime components
how the pieces fit together
```

It also tells the agent not to invent unrelated technologies.

This makes `vix agent analyze` better suited for real project understanding than a generic chat prompt.

## `vix agent scan`

Use `scan` to inspect which files the agent can see in a workspace.

```bash
vix agent scan .
```

Output includes:

```txt
workspace path
number of files
number of skipped files
whether scan was truncated
file list with sizes
```

Example output shape:

```txt
Scanning agent
  provider: ollama
  model: llama3
  timeout: 120000ms
  workspace: .

agent [============================] done
✔ Scanned workspace

summary:
  workspace: .
  files: 42
  skipped: 8
  truncated: no

files:
  • CMakeLists.txt (1200 bytes)
  • src/main.cpp (900 bytes)
  • include/vix/app.hpp (2400 bytes)
```

`scan` is useful before `analyze` when you want to confirm that the workspace is being read correctly.

## Workspace

The default workspace is:

```txt
.
```

You can set it with:

```bash
vix agent ask "Explain this project" --workspace .
```

or:

```bash
vix agent ask "Explain this project" -w .
```

For `analyze` and `scan`, you can also pass the workspace positionally:

```bash
vix agent analyze .
vix agent scan .
```

## Provider

Set the provider with:

```bash
vix agent ask "Explain Vix.cpp" --provider ollama
```

If not provided, Vix loads the provider from the environment.

Default behavior:

```txt
VIX_AGENT_PROVIDER or ollama
```

## Model

Set the model with:

```bash
vix agent ask "Explain Vix.cpp" --model llama3
```

For a lighter local demo:

```bash
ollama pull qwen2.5-coder:1.5b

vix agent ask "Explain Vix.cpp" \
  --model qwen2.5-coder:1.5b \
  --timeout 120000
```

If not provided, Vix loads the model from the environment.

Default behavior:

```txt
VIX_AGENT_MODEL or llama3
```

## Model URL

Set the model endpoint with:

```bash
vix agent ask "Explain Vix.cpp" \
  --model-url http://127.0.0.1:11434
```

If not provided, Vix loads the endpoint from:

```txt
VIX_AGENT_MODEL_URL
```

When the endpoint is set, Vix prints it in the agent header.

## Timeout

Set the request timeout in milliseconds:

```bash
vix agent ask "Explain Vix.cpp" --timeout 120000
```

The timeout must be a positive integer value.

Examples:

```bash
vix agent ask "Explain this project" --timeout 60000
vix agent analyze . --timeout 180000
vix agent ask "Explain this code" --timeout 300000
```

For slow local CPU models, use a larger timeout:

```bash
vix agent ask "Explain this project" --timeout 300000
```

## File reading

By default, file reading is enabled.

```txt
allow file read: yes
```

Disable workspace file reading with:

```bash
vix agent ask "Explain this project" --no-file-read
```

Use this when you want a pure prompt answer without letting the agent read files from the workspace.

## Process execution

By default, process execution is disabled.

```txt
allow process: no
```

Enable safe command execution with:

```bash
vix agent ask "Run vix tests if useful" --allow-process
```

When enabled, Vix restricts allowed programs to:

```txt
vix
cmake
ninja
git
ls
cat
echo
```

This allows the agent to use safe local command tools while keeping the command surface limited.

## File writing

File writing is disabled in the current command configuration.

```txt
allow file write: no
```

There is no CLI flag in the current implementation to enable file writing.

## Cache

The agent cache is enabled by default.

```txt
use cache: yes
```

Disable it with:

```bash
vix agent ask "Explain Vix.cpp" --no-cache
```

When a response comes from cache, Vix prints:

```txt
details:
  cache: hit
```

Use `--no-cache` when you want a fresh model response.

## Memory and run history

Memory persistence is enabled by default.

```txt
persist memory: yes
```

Disable it with:

```bash
vix agent ask "Explain Vix.cpp" --no-memory
```

This disables run history and memory persistence for the request.

## Details output

After a successful agent request, Vix can print details such as:

```txt
run id
cache hit
tools used
```

Example shape:

```txt
details:
  run id: agent-run-id
  cache: hit
  tools: 2
    ✔ file.read
    ✔ command.run
```

A run ID is useful when debugging agent behavior or looking at saved run history.

## Tool output

If tools are used, Vix prints the number of tools and their status.

Example:

```txt
tools: 2
  ✔ file.read
  ✖ command.run
```

This makes agent behavior more transparent.

## Ollama demo

The simplest local demo uses Ollama.

First, pull a small model:

```bash
ollama pull qwen2.5-coder:1.5b
```

Then run:

```bash
vix agent ask "Explain Vix.cpp" \
  --model qwen2.5-coder:1.5b \
  --timeout 120000
```

This is a good local setup for demos because the model is lighter than larger default models.

## Slow model hint

If an Ollama request fails because the model is slow, Vix prints hints like:

```txt
If the model is slow on CPU, try a smaller prompt or `--timeout 300000`.
For a lighter local demo, run `ollama pull qwen2.5-coder:1.5b`.
Then use `--model qwen2.5-coder:1.5b`.
```

Use:

```bash
vix agent ask "Explain Vix.cpp" \
  --model qwen2.5-coder:1.5b \
  --timeout 300000
```

## Environment-driven configuration

`vix agent` loads base configuration from the environment.

Important environment variables include:

```txt
VIX_AGENT_PROVIDER
VIX_AGENT_MODEL
VIX_AGENT_MODEL_URL
```

CLI options override environment values.

Example:

```bash
VIX_AGENT_PROVIDER=ollama \
VIX_AGENT_MODEL=qwen2.5-coder:1.5b \
vix agent ask "Explain Vix.cpp" --timeout 120000
```

## Agent header

When a request starts, Vix prints a task header with metadata.

Example shape:

```txt
Asking agent
  provider: ollama
  model: qwen2.5-coder:1.5b
  timeout: 120000ms
  workspace: .
  endpoint: http://127.0.0.1:11434
```

For analyze:

```txt
Analyzing agent
```

For scan:

```txt
Scanning agent
```

## Progress output

Vix prints an agent progress line.

Success:

```txt
agent [============================] done
```

Failure:

```txt
agent [============================] failed
```

Then it prints the final result or error.

## Options

| Option                   | Description                                                    |
| ------------------------ | -------------------------------------------------------------- |
| `-w, --workspace <path>` | Workspace directory. Default: `.`                              |
| `--provider <name>`      | Model provider. Default from `VIX_AGENT_PROVIDER` or `ollama`. |
| `--model <name>`         | Model name. Default from `VIX_AGENT_MODEL` or `llama3`.        |
| `--model-url <url>`      | Model endpoint. Default from `VIX_AGENT_MODEL_URL`.            |
| `--timeout <ms>`         | Model request timeout in milliseconds.                         |
| `--allow-process`        | Allow the safe `command.run` tool.                             |
| `--no-file-read`         | Disable workspace file reading.                                |
| `--no-cache`             | Disable cache.                                                 |
| `--no-memory`            | Disable run history and memory persistence.                    |
| `-h, --help`             | Show command help.                                             |

## Commands reference

| Command                                             | Description                                 |
| --------------------------------------------------- | ------------------------------------------- |
| `vix agent ask "prompt"`                            | Ask a direct question.                      |
| `vix agent analyze .`                               | Analyze the current project.                |
| `vix agent scan .`                                  | Scan the current workspace.                 |
| `vix agent ask "prompt" --timeout 120000`           | Run with a custom timeout.                  |
| `vix agent ask "prompt" --model qwen2.5-coder:1.5b` | Use a specific model.                       |
| `vix agent ask "prompt" --allow-process`            | Allow safe local command execution.         |
| `vix agent ask "prompt" --no-cache`                 | Disable cached responses.                   |
| `vix agent ask "prompt" --no-memory`                | Disable run history and memory persistence. |
| `vix agent ask "prompt" --no-file-read`             | Disable workspace file reading.             |

## Common workflows

### Ask a simple question

```bash
vix agent ask "Explain Vix.cpp in simple words"
```

### Ask with a longer timeout

```bash
vix agent ask "Explain the Vix build system" --timeout 180000
```

### Use a lighter local model

```bash
ollama pull qwen2.5-coder:1.5b

vix agent ask "Explain this project" \
  --model qwen2.5-coder:1.5b \
  --timeout 120000
```

### Analyze a project

```bash
vix agent analyze .
```

### Analyze with a custom prompt

```bash
vix agent analyze . "Explain the modules, CLI commands, and build system"
```

### Scan a workspace

```bash
vix agent scan .
```

### Ask without reading files

```bash
vix agent ask "Explain C++ RAII" --no-file-read
```

### Ask and allow safe commands

```bash
vix agent ask "Run vix tests if useful and explain failures" --allow-process
```

### Disable cache

```bash
vix agent ask "Explain this project again" --no-cache
```

### Disable memory

```bash
vix agent ask "Give a fresh explanation" --no-memory
```

## Common mistakes

### Running `ask` without a prompt

Wrong:

```bash
vix agent ask
```

Correct:

```bash
vix agent ask "Explain Vix.cpp"
```

### Passing an unknown subcommand

Wrong:

```bash
vix agent explain "Vix.cpp"
```

Correct:

```bash
vix agent ask "Explain Vix.cpp"
```

Supported subcommands:

```txt
ask
analyze
scan
```

### Expecting process execution by default

By default, the agent cannot run commands.

To allow safe commands:

```bash
vix agent ask "Run vix tests if useful" --allow-process
```

### Expecting file writing

The current CLI command does not enable file writing.

The agent can read files by default, but file writing remains disabled.

### Using too small a timeout

Large local models can be slow on CPU.

Use:

```bash
vix agent ask "Explain this project" --timeout 300000
```

or use a smaller model:

```bash
ollama pull qwen2.5-coder:1.5b
```

### Forgetting to run Ollama

If using Ollama, make sure Ollama is installed and the model is available.

Example:

```bash
ollama pull qwen2.5-coder:1.5b
```

Then:

```bash
vix agent ask "Explain Vix.cpp" --model qwen2.5-coder:1.5b
```

### Expecting scan to answer questions

`vix agent scan` only scans and prints workspace file information.

Use `analyze` for explanation:

```bash
vix agent analyze .
```

### Confusing workspace with prompt

For `analyze`, the first positional value is the workspace.

Example:

```bash
vix agent analyze . "Explain the architecture"
```

Here:

```txt
.                         workspace
"Explain the architecture" prompt
```

For `ask`, all positional arguments form the prompt.

## Troubleshooting

### `Agent config error`

The agent configuration failed validation.

Check:

```txt
provider
model
model URL
timeout
environment variables
```

Then try:

```bash
vix agent ask "Explain Vix.cpp" \
  --provider ollama \
  --model qwen2.5-coder:1.5b \
  --timeout 120000
```

### `Agent workspace error`

The workspace could not be opened.

Check that the path exists:

```bash
ls .
```

Then run:

```bash
vix agent scan .
```

or:

```bash
vix agent analyze --workspace .
```

### `Agent scan failed`

The scanner failed while reading the workspace.

Check:

```txt
workspace permissions
ignored or inaccessible files
very large project size
file read settings
```

Try:

```bash
vix agent scan . --no-cache
```

### `Agent request failed`

The model request failed.

Common causes:

```txt
model provider is not running
model is not pulled
timeout is too small
model endpoint is wrong
workspace is too large
```

For Ollama:

```bash
ollama pull qwen2.5-coder:1.5b

vix agent ask "Explain Vix.cpp" \
  --model qwen2.5-coder:1.5b \
  --timeout 300000
```

### Cached answer is not what you want

Disable cache:

```bash
vix agent ask "Explain this again" --no-cache
```

### Agent cannot run tests

Allow process execution:

```bash
vix agent ask "Run vix tests if useful" --allow-process
```

Only safe allowed programs are available when process execution is enabled.

### Agent cannot read project files

Make sure you did not pass:

```bash
--no-file-read
```

Then run:

```bash
vix agent analyze .
```

## Best practices

Use `vix agent scan .` first when debugging workspace visibility.

Use `vix agent analyze .` for repository-level explanations.

Use `vix agent ask` for direct questions.

Use `--timeout 120000` or higher for local models.

Use `qwen2.5-coder:1.5b` for lighter local demos.

Keep file reading enabled for project analysis.

Use `--allow-process` only when you want the agent to run safe commands.

Use `--no-cache` when you need a fresh response.

Use `--no-memory` when you do not want run history or memory persistence.

Prefer small, focused prompts for faster local model responses.

## Related commands

| Command      | Purpose                                                         |
| ------------ | --------------------------------------------------------------- |
| `vix build`  | Build the project before asking the agent about build behavior. |
| `vix check`  | Validate project health.                                        |
| `vix tests`  | Run project tests.                                              |
| `vix doctor` | Diagnose environment issues.                                    |
| `vix info`   | Inspect Vix paths, caches, and local state.                     |
| `vix dev`    | Run the project in development mode.                            |

## Next step

Continue with project diagnostics.

[Open the vix doctor guide](/cli/doctor)
