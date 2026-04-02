# Vix Shell Completion

Enable command auto-completion for Vix in Bash.

---

## Overview

Vix can generate shell completion scripts for Bash.

This allows:

- faster command typing
- fewer mistakes
- discoverability of commands

---

## Quick Start

Load completion for the current shell session:

```bash
source <(vix completion bash)
```

Now try:

```bash
vix bu<TAB>
vix ins<TAB>
vix help ru<TAB>
```

---

## Persist It

To enable completion automatically:

```bash
vix completion bash > ~/.vix-completion.bash
echo 'source ~/.vix-completion.bash' >> ~/.bashrc
source ~/.bashrc
```

---

## What It Completes

Supports:

- top-level commands
- `vix help <command>`

Examples:

```bash
vix bu<TAB>        # build
vix compl<TAB>     # completion
vix help ru<TAB>   # run
```

---

## How It Works

`vix completion bash` prints a Bash completion script to stdout.

To activate it:

```bash
source <(vix completion bash)
```

Running only:

```bash
vix completion bash
```

will NOT enable completion.

---

## Troubleshooting

If TAB does not work:

### 1. Ensure script is loaded

```bash
source <(vix completion bash)
```

### 2. Ensure Bash is used

```bash
echo $SHELL
```

### 3. Reload config

```bash
source ~/.bashrc
```

### 4. Check output

```bash
vix completion bash
```

You should see `_vix_completions`.

---

## Example Session

```bash
source <(vix completion bash)

vix bu<TAB>
vix help in<TAB>
```

---

## Notes

- Completion is generated from registered Vix commands
- Always stays in sync with CLI
- No external Bash completion tools required

---

## License

MIT License
Copyright (c) Gaspard Kirira

