# vix completion

`vix completion` generates a shell completion script.

Use it when you want your shell to autocomplete Vix commands, subcommands, and options.

## Usage

```bash
vix completion [bash]
```

## What it does

`vix completion` prints a shell completion script. You redirect that output into a file, then source the file from your shell.

## Basic usage

```bash
vix completion bash > ~/.vix-completion.bash
source ~/.vix-completion.bash
```

## Recommended persistent Bash setup

```bash
vix completion bash > ~/.vix-completion.bash
echo 'source ~/.vix-completion.bash' >> ~/.bashrc
source ~/.bashrc
```

After that, open a new terminal and try `vix <Tab>`.

## One-time usage

```bash
source <(vix completion bash)
```

This does not persist after closing the terminal.

## Update completions after upgrading Vix

```bash
vix upgrade
vix completion bash > ~/.vix-completion.bash
source ~/.bashrc
```

## Shell support

The documented completion target is `bash`:

```bash
vix completion bash
```

## Common workflows

```bash
# Generate Bash completion
vix completion bash

# Save completion file
vix completion bash > ~/.vix-completion.bash

# Load completion in current shell
source ~/.vix-completion.bash

# Install persistent Bash completion
vix completion bash > ~/.vix-completion.bash
echo 'source ~/.vix-completion.bash' >> ~/.bashrc
source ~/.bashrc

# Regenerate after upgrade
vix upgrade
vix completion bash > ~/.vix-completion.bash
source ~/.bashrc
```

## Common mistakes

### Forgetting to source the file

```bash
# Only writes the script
vix completion bash > ~/.vix-completion.bash

# You still need to load it
source ~/.vix-completion.bash
```

### Expecting completion to persist without editing shell config

Add to `~/.bashrc`:

```bash
echo 'source ~/.vix-completion.bash' >> ~/.bashrc
```

### Forgetting to regenerate after upgrade

```bash
vix completion bash > ~/.vix-completion.bash
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix --help` | Show global CLI help |
| `vix help <command>` | Show command help |
| `vix upgrade` | Upgrade Vix |
| `vix doctor` | Check environment health |
| `vix info` | Inspect Vix paths and local state |

## Next step

Return to the command overview.

[Open all CLI commands](/cli/commands)
