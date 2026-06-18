# vix completion

`vix completion` generates a shell completion script for the Vix CLI.

Use it when you want your shell to autocomplete Vix commands.

## Usage

```bash
vix completion
vix completion bash
```

Both commands generate Bash completion.

## What it does

`vix completion` prints a shell completion script to standard output.

You can either load it for the current terminal session or save it to a file and source it from your shell configuration.

The generated completion script currently supports:

- top-level Vix commands
- `vix help <command>` completion
- Bash completion through the `complete` builtin

## Supported shells

| Shell      | Status            |
| ---------- | ----------------- |
| Bash       | Supported         |
| Zsh        | Not supported yet |
| Fish       | Not supported yet |
| PowerShell | Not supported yet |

The documented target is:

```bash
vix completion bash
```

Calling `vix completion` without a shell also generates Bash completion.

## Basic usage

Generate and load completion for the current shell session:

```bash
vix completion bash > ~/.vix-completion.bash
source ~/.vix-completion.bash
```

Then try:

```bash
vix <Tab>
```

You should see available Vix commands.

## One-time usage

Use this when you only want completion for the current terminal session:

```bash
source <(vix completion bash)
```

This is temporary.

When you close the terminal, the completion is gone.

## Persistent Bash setup

Use this to enable Vix completion every time you open a Bash terminal:

```bash
vix completion bash > ~/.vix-completion.bash
echo 'source ~/.vix-completion.bash' >> ~/.bashrc
source ~/.bashrc
```

After that, open a new terminal and try:

```bash
vix <Tab>
```

## Safer persistent setup

If you want to avoid adding the same line several times to `~/.bashrc`, use:

```bash
vix completion bash > ~/.vix-completion.bash

grep -qxF 'source ~/.vix-completion.bash' ~/.bashrc || \
  echo 'source ~/.vix-completion.bash' >> ~/.bashrc

source ~/.bashrc
```

## System-wide Bash setup

If you want to install completion for all users on a Linux system, you can write it to the Bash completion directory:

```bash
vix completion bash | sudo tee /etc/bash_completion.d/vix > /dev/null
```

Then start a new shell.

This requires Bash completion support to be installed on the system.

## Update completions after upgrading Vix

The completion script is generated from the commands registered in the Vix CLI.

After upgrading Vix, regenerate the completion file:

```bash
vix upgrade
vix completion bash > ~/.vix-completion.bash
source ~/.bashrc
```

This makes sure newly added commands can appear in completion.

## How command completion works

The generated Bash script defines a `_vix_completions` function.

At the first command position, it completes available Vix commands:

```bash
vix <Tab>
```

It also completes command names after `vix help`:

```bash
vix help <Tab>
```

The command list is collected from the Vix command dispatcher at generation time.

That means the generated file reflects the installed Vix binary that produced it.

## Current behavior

The current completion implementation focuses on top-level commands.

It does not yet complete every option of every subcommand.

For example, it can help with:

```bash
vix bu<Tab>
vix he<Tab>
vix help bu<Tab>
```

But it does not yet provide deep option completion such as:

```bash
vix build --pr<Tab>
vix run --sa<Tab>
vix registry sy<Tab>
```

Those can be added later when command-specific completion metadata is available.

## Basic workflows

```bash
# Print Bash completion script
vix completion bash

# Save completion script
vix completion bash > ~/.vix-completion.bash

# Load completion in current shell
source ~/.vix-completion.bash

# Install persistent completion
vix completion bash > ~/.vix-completion.bash
echo 'source ~/.vix-completion.bash' >> ~/.bashrc
source ~/.bashrc

# Regenerate after upgrading Vix
vix upgrade
vix completion bash > ~/.vix-completion.bash
source ~/.bashrc
```

## Common mistakes

### Forgetting to source the generated file

This only writes the script:

```bash
vix completion bash > ~/.vix-completion.bash
```

You still need to load it:

```bash
source ~/.vix-completion.bash
```

### Expecting completion to persist automatically

This only works for the current terminal:

```bash
source <(vix completion bash)
```

For persistent completion, add it to `~/.bashrc`:

```bash
echo 'source ~/.vix-completion.bash' >> ~/.bashrc
```

### Forgetting to regenerate after upgrade

If you upgrade Vix and new commands were added, regenerate the completion file:

```bash
vix completion bash > ~/.vix-completion.bash
```

### Using an unsupported shell target

This is supported:

```bash
vix completion bash
```

This is not supported yet:

```bash
vix completion zsh
vix completion fish
vix completion powershell
```

Unsupported shells return an error.

## Troubleshooting

### Completion does not work

First, confirm that the completion file exists:

```bash
ls ~/.vix-completion.bash
```

Then source it again:

```bash
source ~/.vix-completion.bash
```

Try completion:

```bash
vix <Tab>
```

### Bash completion is not installed

On some Linux systems, Bash completion support may not be loaded by default.

Install it using your system package manager.

Ubuntu or Debian:

```bash
sudo apt install bash-completion
```

Then restart your terminal.

### Completion file is outdated

Regenerate it:

```bash
vix completion bash > ~/.vix-completion.bash
source ~/.bashrc
```

## Related commands

| Command              | Purpose                           |
| -------------------- | --------------------------------- |
| `vix --help`         | Show global CLI help              |
| `vix help <command>` | Show help for a specific command  |
| `vix info`           | Inspect Vix paths and local state |
| `vix doctor`         | Check environment health          |
| `vix upgrade`        | Upgrade Vix                       |
| `vix commands`       | Open the command overview         |

## Next step

Return to the command overview.

[Open all CLI commands](/cli/commands)
