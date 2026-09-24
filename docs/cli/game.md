# vix game

`vix game` provides game project tools for Vix.cpp projects using the `vix/game` runtime.

Use it when you want to export a game project into a distributable directory.

```bash
vix game export
```

## Overview

`vix game` is the command namespace for game-related workflows.

The current command is:

```txt
vix game export
```

It prepares a game project for distribution by exporting project files into an output directory.

The export can include:

```txt
assets/
game.package.json
README.md
export.json
```

The command is designed for projects created with the Vix game template or projects using the `vix/game` module.

## Usage

```bash
vix game export [options]
```

## Basic examples

```bash
# Export the current game project
vix game export

# Export from an explicit project root
vix game export --project-root .

# Export to a specific directory
vix game export --output dist

# Override the exported game name
vix game export --name mario-release

# Refuse to overwrite existing export output
vix game export --no-overwrite

# Export without copying assets
vix game export --no-assets
```

## What it does

`vix game export` exports a game project into a distributable directory.

It uses a game export configuration with these defaults:

```txt
project root      .
output directory  dist
asset directory   assets
package file      game.package.json
readme file       README.md
name              game
copy assets       yes
copy package      yes
copy README       yes, when it exists
overwrite         yes
```

The exporter reads project data, copies game resources, and writes export metadata.

## Default export

Run:

```bash
vix game export
```

Default behavior:

```txt
project root      .
output directory  dist
export name       game
assets            copied from assets/
package file      copied from game.package.json
README            copied if README.md exists
overwrite         enabled
```

## Export result

After a successful export, Vix prints:

```txt
Game exported.
Output             <path>
Name               <name>
Version            <version>
Asset root         <asset-root>
Copied files       <count>
Copied directories <count>
```

Example output shape:

```txt
✔ Game exported.
Output             dist/game
Name               game
Version            0.1.0
Asset root         assets
Copied files       12
Copied directories 3
```

## Project root

Use:

```bash
vix game export --project-root <path>
```

or:

```bash
vix game export --project-root=<path>
```

Example:

```bash
vix game export --project-root ./examples/platformer
```

This tells Vix where the game project is located.

Default:

```txt
.
```

## Output directory

Use:

```bash
vix game export --output <path>
```

or:

```bash
vix game export --output=<path>
```

Example:

```bash
vix game export --output dist
```

The output directory is where the exported game files are written.

Default:

```txt
dist
```

## Export name

Use:

```bash
vix game export --name <name>
```

or:

```bash
vix game export --name=<name>
```

Example:

```bash
vix game export --name mario-release
```

This overrides the export name.

If no name is provided, the exporter can use the package name from `game.package.json` when available.

Otherwise, it uses:

```txt
game
```

## Assets

By default, Vix copies assets from:

```txt
assets/
```

Example structure:

```txt
my-game/
├── assets/
│   ├── player.png
│   ├── coin.png
│   └── background.png
├── game.package.json
└── README.md
```

Export:

```bash
vix game export
```

The exported output includes the assets.

## Disable asset copying

Use:

```bash
vix game export --no-assets
```

This exports the game without copying the `assets/` directory.

Use this when assets are handled by another packaging step or when you only want metadata output.

## Package file

The default game package file is:

```txt
game.package.json
```

When present, the exporter can use it for metadata such as:

```txt
name
version
asset root
```

It is copied into the exported output by default.

## README file

The default README file is:

```txt
README.md
```

When it exists, the exporter copies it into the exported output.

This is useful for sharing a game build with basic project information.

## Overwrite behavior

By default, `vix game export` allows overwriting an existing export directory.

Default:

```txt
overwrite yes
```

To prevent overwriting, use:

```bash
vix game export --no-overwrite
```

If the output already exists, the export fails instead of replacing it.

Use this for safer release workflows.

## Game support requirement

`vix game export` requires Vix to be built with game support enabled.

If game support is not enabled, Vix prints:

```txt
Game support is not enabled in this Vix build.
Rebuild Vix with -DVIX_ENABLE_GAME=ON.
```

Fix:

```bash
cmake -DVIX_ENABLE_GAME=ON ...
```

Then rebuild Vix.

## Recommended game project structure

A simple Vix game project can look like this:

```txt
my-game/
├── src/
│   ├── main.cpp
│   ├── MainScene.cpp
│   └── Player.cpp
├── include/
│   └── my-game/
├── assets/
│   ├── player.png
│   └── coin.png
├── game.package.json
├── README.md
└── vix.app
```

The important export-related files are:

```txt
assets/
game.package.json
README.md
```

## Example `game.package.json`

A simple package file can look like this:

```json
{
  "name": "mario",
  "version": "0.1.0",
  "asset_root": "assets"
}
```

Then run:

```bash
vix game export
```

The result uses:

```txt
name       mario
version    0.1.0
asset root assets
```

## Export workflow

Recommended local workflow:

```bash
vix build
vix game export
```

For release builds:

```bash
vix build --preset release
vix game export --output dist --name mario-release
```

For safer release exports:

```bash
vix build --preset release
vix game export --output dist --name mario-release --no-overwrite
```

## Export without assets

```bash
vix game export --no-assets
```

Use this when:

```txt
assets are already copied elsewhere
assets are embedded later
you only want export metadata
you are testing export logic
```

## Export from another folder

```bash
vix game export --project-root ./games/mario --output ./dist
```

This exports:

```txt
./games/mario
```

into:

```txt
./dist
```

## Options

| Option                  | Description                      |
| ----------------------- | -------------------------------- |
| `--project-root <path>` | Project root. Default: `.`       |
| `--project-root=<path>` | Same as `--project-root <path>`. |
| `--output <path>`       | Override output directory.       |
| `--output=<path>`       | Same as `--output <path>`.       |
| `--name <name>`         | Override export name.            |
| `--name=<name>`         | Same as `--name <name>`.         |
| `--no-overwrite`        | Fail if output already exists.   |
| `--no-assets`           | Do not copy assets.              |
| `-h, --help`            | Show command help.               |

## Commands reference

| Command                                | Description                                   |
| -------------------------------------- | --------------------------------------------- |
| `vix game export`                      | Export the current game project.              |
| `vix game export --project-root .`     | Export from the current directory explicitly. |
| `vix game export --output dist`        | Export into `dist`.                           |
| `vix game export --name mario-release` | Override export name.                         |
| `vix game export --no-overwrite`       | Fail if export output exists.                 |
| `vix game export --no-assets`          | Export without copying assets.                |

## Common workflows

### Export current game

```bash
vix game export
```

### Build and export

```bash
vix build
vix game export
```

### Release build and export

```bash
vix build --preset release
vix game export --output dist --name mario-release
```

### Export without overwriting

```bash
vix game export --no-overwrite
```

### Export without assets

```bash
vix game export --no-assets
```

### Export another project

```bash
vix game export --project-root ./games/mario --output ./dist
```

## Common mistakes

### Running without game support

If you see:

```txt
Game support is not enabled in this Vix build.
```

rebuild Vix with:

```txt
-DVIX_ENABLE_GAME=ON
```

### Expecting `vix game export` to build the game

`vix game export` exports project files.

It does not replace:

```bash
vix build
```

Recommended:

```bash
vix build --preset release
vix game export
```

### Forgetting assets

If your exported game has no textures, sprites, sounds, or images, check that your project has:

```txt
assets/
```

and that you did not pass:

```bash
--no-assets
```

### Overwriting an export accidentally

By default, overwrite is enabled.

For safer release exports, use:

```bash
vix game export --no-overwrite
```

### Passing the wrong project root

Wrong:

```bash
vix game export --project-root ./dist
```

Correct:

```bash
vix game export --project-root ./my-game
```

The project root should be the game project folder, not the output folder.

### Expecting package metadata without `game.package.json`

If `game.package.json` is missing, Vix can still export, but metadata falls back to defaults.

Default values include:

```txt
name    game
version 0.1.0
assets  assets
```

## Troubleshooting

### `Unknown game command`

Use:

```bash
vix game export
```

Current supported command:

```txt
export
```

### `Game export failed`

The exporter returns a structured game error.

Vix prints:

```txt
Game export failed.
<error message>
```

Check:

```txt
project root exists
output directory is writable
assets directory exists when assets are enabled
game.package.json is valid when present
existing output is allowed or --no-overwrite is not used
```

### Output already exists

If you used:

```bash
vix game export --no-overwrite
```

and the output exists, remove the old output or choose another name/output directory.

Example:

```bash
vix game export --name mario-release-2
```

### Assets not copied

Check that assets exist under:

```txt
assets/
```

and do not use:

```bash
--no-assets
```

### Wrong export name

Pass the name explicitly:

```bash
vix game export --name mario-release
```

or set the name inside:

```txt
game.package.json
```

## Best practices

Run `vix build` before exporting a release.

Use `--name` for release exports.

Use `--output dist` for predictable output.

Use `--no-overwrite` for release workflows.

Keep `game.package.json` accurate.

Keep assets under `assets/` unless your game package configuration says otherwise.

Use `--no-assets` only when another step handles assets.

Do not edit exported files manually for release builds. Regenerate the export instead.

## Related commands

| Command      | Purpose                                              |
| ------------ | ---------------------------------------------------- |
| `vix build`  | Build the game project.                              |
| `vix dev`    | Run the project during development.                  |
| `vix run`    | Run the project directly.                            |
| `vix pack`   | Package a Vix project into a distributable artifact. |
| `vix verify` | Verify a package artifact.                           |
| `vix cache`  | Store a package locally.                             |

## Next step

Continue with project packaging.

[Open the vix pack guide](/cli/pack)
