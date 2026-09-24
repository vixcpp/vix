# Game Template

The game template creates a Vix game project.
Use it when you want to build a small game, a simulation, an interactive prototype, a real-time visualization, or the beginning of a custom engine on top of Vix.cpp.

Create a game project with:

```bash
vix new mario --game
```

You can also use:

```bash
vix new mario --template game
```

## What this template is for

Use the game template when you want:

- a C++ game project
- a game prototype
- a simulation
- an interactive tool
- a custom engine foundation
- a 2D rendering experiment
- a real-time visualization
- a project with scenes, frame updates, assets, and game metadata

This template is not trying to be Unity, Unreal, or Godot.
It gives you a clean Vix game foundation so you can build your own architecture.

## What `vix/game` provides

`vix/game` is a game application foundation for Vix.cpp.

It provides building blocks such as:

- application lifecycle
- game loop
- frame timing
- fixed update support
- event system
- scene management
- ECS-style registry
- entities, components, systems
- asset management
- asset cache
- async asset loading
- background job system
- input system
- window abstraction
- renderer abstraction
- 2D rendering foundation
- runtime context
- editor runtime foundation
- scripting runtime foundation
- audio runtime foundation
- physics runtime foundation
- game package metadata
- game export workflow

The important idea is:

```txt
Vix gives you the runtime foundation.
You keep control of your game architecture.
```

## Design used by this template

The game template uses a **runtime-first game architecture**.

The generated project starts with:

```txt
App
  -> GameRuntime
      -> SceneManager
          -> MainScene
              -> on_load()
              -> on_update(frame)
```

The generated app runs a small scene, prints frame updates, then stops after a few frames.

This is intentional.

It proves that:

- the project builds
- the game runtime starts
- the scene system works
- the update loop runs
- the game can stop cleanly

## Quick start

Create the project:

```bash
vix new mario --game
```

Enter the project:

```bash
cd mario
```

Build it:

```bash
vix build
```

Run it:

```bash
vix run
```

Expected output shape:

```txt
Main scene loaded
frame: 0
frame: 1
frame: 2
frame: 3
frame: 4
frame: 5
```

The generated game is short-lived by default.

It runs a few frames and exits.

That makes it easy to verify the template from the terminal.

## Generated structure

A generated game project looks like this:

```txt
mario/
├── assets/
├── src/
│   └── main.cpp
├── game.package.json
├── README.md
├── vix.app
└── vix.json
```

## What each file does

| File or folder      | Role                                                     |
| ------------------- | -------------------------------------------------------- |
| `src/main.cpp`      | Game entry point and first scene.                        |
| `assets/`           | Game assets such as images, text files, sounds, or maps. |
| `game.package.json` | Game metadata used by packaging and export workflows.    |
| `vix.app`           | Vix build manifest for the game executable.              |
| `vix.json`          | Project metadata and tasks.                              |
| `README.md`         | Generated project documentation.                         |

## `src/main.cpp`

The generated game starts with one scene.

A simplified version looks like this:

```cpp
#include <vix/game/all.hpp>
#include <vix/print.hpp>

class MainScene final : public vix::game::Scene
{
public:
  vix::game::GameBoolResult on_load() override
  {
    vix::print("Main scene loaded");
    return vix::game::Scene::on_load();
  }

  void on_update(const vix::game::Frame &frame) override
  {
    vix::print("frame:", frame.index);

    if (frame.index >= 5)
    {
      app().stop();
    }
  }
};

int main()
{
  vix::game::App app;
  app.set_title("mario");

  vix::game::GameRuntime runtime(app);

  auto runtime_init = runtime.init();
  if (!runtime_init)
  {
    vix::print("runtime init failed:", runtime_init.error().message());
    return 1;
  }

  auto scene = app.scenes().create<MainScene>("main");
  if (!scene)
  {
    vix::print("scene creation failed:", scene.error().message());
    return 1;
  }

  auto active = app.scenes().set_active("main");
  if (!active)
  {
    vix::print("scene activation failed:", active.error().message());
    return 1;
  }

  auto result = app.run();
  if (!result)
  {
    vix::print("game failed:", result.error().message());
    return 1;
  }

  return 0;
}
```

## Main concepts

| Concept            | Role                                                              |
| ------------------ | ----------------------------------------------------------------- |
| `App`              | Main game application facade.                                     |
| `GameRuntime`      | Initializes and coordinates runtime systems.                      |
| `Scene`            | Represents a game state such as menu, gameplay, pause, or editor. |
| `SceneManager`     | Creates, stores, activates, and updates scenes.                   |
| `Frame`            | Carries frame timing and frame index.                             |
| `on_load()`        | Runs when the scene loads.                                        |
| `on_update(frame)` | Runs every frame.                                                 |
| `app().stop()`     | Stops the game loop.                                              |

## Runtime flow

The generated flow is:

```txt
main()
  -> create App
  -> create GameRuntime
  -> runtime.init()
  -> create MainScene
  -> set active scene
  -> app.run()
      -> scene.on_load()
      -> scene.on_update(frame)
  -> app exits
```

This gives you the base loop needed by games and simulations.

## `game.package.json`

`game.package.json` describes the game project.

Generated shape:

```json
{
  "name": "mario",
  "version": "0.1.0",
  "author": "",
  "entry_scene": "main",
  "asset_root": "assets",
  "output_dir": "dist",
  "scenes": ["main"],
  "assets": []
}
```

Use it for:

- game name
- version
- author
- entry scene
- asset root
- output directory
- scene list
- asset list

This file is used by game packaging and export workflows.

## `assets/`

Put game assets in:

```txt
assets/
```

Examples:

```txt
assets/player.png
assets/background.png
assets/level-01.json
assets/dialogue.txt
assets/audio/theme.ogg
```

The game export workflow can scan this directory and generate export metadata.

## `vix.app`

The generated game project uses `vix.app` as its build manifest.

Example shape:

```txt
name = "mario"
type = "executable"
standard = "c++20"

sources = [
  "src/main.cpp",
]

include_dirs = [
  "src",
]

compile_features = [
  "cxx_std_20",
]

packages = [
  "vix",
]

links = [
  "vix::game",
  "vix::io",
]

resources = [
  "assets=assets",
  "game.package.json=game.package.json",
]

output_dir = "bin"
```

The important links are:

```txt
vix::game
vix::io
```

The resources make assets and game metadata available to the built executable.

## `vix.json`

The generated game project includes tasks such as:

```json
{
  "tasks": {
    "dev": "vix run",
    "build": "vix build",
    "run": "vix run",
    "export": "vix run && vix build",
    "check": {
      "description": "Build and validate the game project",
      "command": "vix build"
    }
  }
}
```

Common commands:

```bash
vix task build
vix task run
vix task check
```

## Build and run

Build:

```bash
vix build
```

Run:

```bash
vix run
```

Run development mode:

```bash
vix dev
```

For generated short-lived games, `vix run` is often enough.

Use `vix dev` when you want Vix to rebuild while you edit.

## Export the game

The game template is prepared for export workflows.

Run:

```bash
vix game export
```

This can generate:

```txt
dist/
  mario/
    assets/
    game.package.json
    README.md
    export.json
```

`export.json` records exported metadata such as:

- game name
- version
- asset root
- output path
- copied files
- exported assets

## Headless vs windowed games

The generated template is terminal-friendly and short-lived.

It does not force a window backend.

That is intentional.

`vix/game` is backend-independent by design.

You can start with the simple runtime and later add:

- SDL window backend
- OpenGL renderer
- input handling
- sprites
- cameras
- textures
- editor tools

For example, a windowed SDL/OpenGL demo uses:

```cpp
#include <memory>

#include <vix/game/all.hpp>
#include <vix/game/backends/sdl/SDLOpenGLRenderer.hpp>
#include <vix/game/backends/sdl/SDLWindow.hpp>
#include <vix/print.hpp>
```

Then it configures:

```cpp
config.window.width = 960;
config.window.height = 540;
config.window.opengl = true;
```

And installs the backends:

```cpp
context
  .set_window_backend(std::make_unique<vix::game::sdl::SDLWindow>())
  .set_renderer_backend(std::make_unique<vix::game::sdl::SDLOpenGLRenderer>());
```

For the full SDL/OpenGL setup, use the dedicated game guide.

## System dependencies for SDL/OpenGL

The default generated game can run without forcing SDL/OpenGL.

If you use the SDL/OpenGL backend, install the required system libraries.

Ubuntu or Debian:

```bash
sudo apt update
sudo apt install -y libsdl2-dev libsdl2-image-dev libgl1-mesa-dev
```

Then build with SDL support if the project or module requires it.

The exact build flags are covered in the game guide.

## How to add a new scene

Create a new scene class.

Example:

```cpp
class MenuScene final : public vix::game::Scene
{
public:
  vix::game::GameBoolResult on_load() override
  {
    vix::print("Menu scene loaded");
    return vix::game::Scene::on_load();
  }

  void on_update(const vix::game::Frame &frame) override
  {
    (void)frame;
  }
};
```

Register it:

```cpp
auto menu = app.scenes().create<MenuScene>("menu");
```

Activate it:

```cpp
app.scenes().set_active("menu");
```

## How to organize a growing game

The generated template starts with everything in `src/main.cpp`.

That is fine for the first run.

When the game grows, split code into folders:

```txt
src/
├── main.cpp
├── scenes/
│   ├── MainScene.hpp
│   ├── MainScene.cpp
│   ├── MenuScene.hpp
│   └── MenuScene.cpp
├── systems/
│   ├── MovementSystem.hpp
│   └── MovementSystem.cpp
├── components/
│   ├── Transform.hpp
│   └── Sprite.hpp
└── assets/
    └── AssetIds.hpp
```

Then update `vix.app`:

```txt
sources = [
  "src/main.cpp",
  "src/scenes/MainScene.cpp",
  "src/scenes/MenuScene.cpp",
  "src/systems/MovementSystem.cpp",
]
```

## How to use assets

Put files in:

```txt
assets/
```

Then keep stable asset names in your code.

Example:

```txt
assets/player.png
assets/levels/level-01.json
```

As the project grows, create:

```txt
src/assets/AssetIds.hpp
```

Example:

```cpp
#pragma once

namespace assets
{
  inline constexpr const char *Player = "assets/player.png";
  inline constexpr const char *Level01 = "assets/levels/level-01.json";
}
```

This avoids hardcoding asset paths everywhere.

## Game template vs application template

Use the game template when the project is a real-time app.

Use the application template when the project is a normal executable or HTTP app.

| Need           | Template                   |
| -------------- | -------------------------- |
| Game loop      | `game`                     |
| Scenes         | `game`                     |
| Frame updates  | `game`                     |
| Assets         | `game`                     |
| Normal C++ app | `application`              |
| HTTP server    | `application` or `backend` |

## Game template vs backend template

Use the game template for interactive real-time applications.

Use the backend template for APIs and services.

| Need                             | Template  |
| -------------------------------- | --------- |
| Game or simulation               | `game`    |
| JSON API                         | `backend` |
| Realtime rendering               | `game`    |
| WebSocket service                | `backend` |
| Asset pipeline                   | `game`    |
| Health checks and API middleware | `backend` |

## Common mistakes

### Expecting a full game engine

`vix/game` is a foundation, not a complete engine.

It gives you structure, runtime, scenes, assets, and abstractions.

You build the higher-level engine or game systems on top.

### Forgetting that generated games are short-lived

The generated game stops after a few frames.

That is normal.

It is a verification template.

When you build a real game, remove this condition:

```cpp
if (frame.index >= 5)
{
  app().stop();
}
```

### Adding new `.cpp` files without updating `vix.app`

If you add source files, add them to:

```txt
sources = [
]
```

in `vix.app`.

### Putting assets outside `assets/`

Keep game assets under:

```txt
assets/
```

This keeps export and packaging workflows simple.

### Trying SDL/OpenGL without system dependencies

Install SDL/OpenGL dependencies before building a windowed renderer project.

Ubuntu or Debian:

```bash
sudo apt install -y libsdl2-dev libsdl2-image-dev libgl1-mesa-dev
```

## What you should remember

The game template creates a Vix game project with:

```txt
App
GameRuntime
Scene
SceneManager
Frame updates
assets/
game.package.json
vix.app
vix.json
```

The generated project is intentionally simple.

It proves the runtime works, then gives you a clean place to grow.

Create a game:

```bash
vix new mario --game
cd mario
vix build
vix run
```

Export later:

```bash
vix game export
```

## Next steps

Continue with:

- [Application template](/templates/application)
- [Backend template](/templates/backend)
- [Game guide](/guides/game)
- [Build and run](/cli/run)
- [vix.app](/guides/vix-app/)
