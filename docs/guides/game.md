# Game Guide

This guide shows how to use `vix/game`.

`vix/game` is the game application foundation of Vix.cpp.

It is not a full game engine.

It gives you the runtime building blocks needed to build games, simulations, interactive tools, prototypes, and custom engines with C++.

## What you will learn

This guide covers:

- creating a game project
- understanding the game loop
- using scenes
- loading assets
- loading assets asynchronously
- using jobs
- handling input
- using window and renderer abstractions
- drawing with `Renderer2D`
- exporting a game project
- moving toward SDL/OpenGL rendering

## Create a game project

Create a game project:

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

The generated game is intentionally short-lived.

It runs a few frames, then exits.

That makes the template easy to test from the terminal.

## Generated project structure

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

| File or folder      | Role                             |
| ------------------- | -------------------------------- |
| `src/main.cpp`      | First game entry point.          |
| `assets/`           | Game assets.                     |
| `game.package.json` | Game metadata.                   |
| `vix.app`           | Build manifest.                  |
| `vix.json`          | Project metadata and tasks.      |
| `README.md`         | Generated project documentation. |

## The mental model

The core model is:

```txt
App
  -> GameRuntime
      -> SceneManager
          -> Scene
              -> on_load()
              -> on_update(frame)
```

The smallest runtime flow is:

```txt
create App
configure App
initialize runtime
create scene
activate scene
run app
shutdown app
```

## Hello game app

This is the smallest useful game setup.

```cpp
#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::App app;

  app.set_title("Hello Vix Game");
  app.set_target_fps(60);

  vix::print("Hello from vix/game");
  vix::print("title:", app.config().title);

  return 0;
}
```

This does not start the game loop yet.

It only creates and configures the app.

## Basic game loop

Use `app.on_update(...)` when you want a direct loop without creating a scene yet.

```cpp
#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::App app;

  app.set_title("Basic Loop");
  app.set_target_fps(60);

  app.on_update(
      [&app](const vix::game::Frame &frame)
      {
        vix::print("frame:", frame.index, "delta_ms:", frame.delta_ms());

        if (frame.index >= 5)
        {
          app.stop();
        }
      });

  auto result = app.run();
  if (!result)
  {
    vix::print("error:", result.error().message());
    return 1;
  }

  return 0;
}
```

Use this style for:

- small experiments
- timing tests
- prototypes
- learning the game loop

For larger projects, use scenes.

## Scenes

A scene represents one state of your game.

Examples:

- menu
- gameplay
- pause screen
- loading screen
- editor view
- simulation view

A scene can define lifecycle hooks:

```txt
on_load
on_enter
on_update
on_fixed_update
on_exit
on_unload
```

Example:

```cpp
#include <vix/game/game.hpp>
#include <vix/print.hpp>

class MainScene final : public vix::game::Scene
{
public:
  MainScene()
      : vix::game::Scene("main")
  {
  }

  vix::game::GameBoolResult on_load() override
  {
    vix::print("main scene loaded");
    return vix::game::Scene::on_load();
  }

  void on_enter() override
  {
    vix::game::Scene::on_enter();
    vix::print("main scene entered");
  }

  void on_update(const vix::game::Frame &frame) override
  {
    vix::print("scene frame:", frame.index);

    if (frame.index >= 5)
    {
      app().stop();
    }
  }

  vix::game::GameBoolResult on_unload() override
  {
    vix::print("main scene unloaded");
    return vix::game::Scene::on_unload();
  }
};

int main()
{
  vix::game::App app;

  auto added = app.scenes().create<MainScene>("main");
  if (!added)
  {
    vix::print("error:", added.error().message());
    return 1;
  }

  auto active = app.scenes().set_active("main");
  if (!active)
  {
    vix::print("error:", active.error().message());
    return 1;
  }

  auto result = app.run();
  if (!result)
  {
    vix::print("error:", result.error().message());
    return 1;
  }

  return 0;
}
```

## Scene serialization

Scene serialization lets you save and restore scene state metadata.

This is useful for editor tools, runtime inspection, and future game project workflows.

```cpp
#include <vix/game/game.hpp>
#include <vix/print.hpp>

class MainScene final : public vix::game::Scene
{
public:
  MainScene()
      : vix::game::Scene("main")
  {
  }
};

class MenuScene final : public vix::game::Scene
{
public:
  MenuScene()
      : vix::game::Scene("menu")
  {
  }
};

static std::unique_ptr<vix::game::Scene> make_scene(
    const std::string &name)
{
  if (name == "main")
  {
    return std::make_unique<MainScene>();
  }

  if (name == "menu")
  {
    return std::make_unique<MenuScene>();
  }

  return nullptr;
}

int main()
{
  vix::game::App app;

  auto menu = app.scenes().create<MenuScene>("menu");
  if (!menu)
  {
    vix::print("failed to create menu scene:", menu.error().message());
    return 1;
  }

  auto main = app.scenes().create<MainScene>("main");
  if (!main)
  {
    vix::print("failed to create main scene:", main.error().message());
    return 1;
  }

  auto active = app.scenes().set_active("main");
  if (!active)
  {
    vix::print("failed to activate scene:", active.error().message());
    return 1;
  }

  auto json = vix::game::SceneSerializer::to_json(app.scenes());

  vix::print("serialized scene count:", app.scenes().size());
  vix::print("active scene:", app.scenes().active_name());

  vix::game::App restored;

  auto restored_result = vix::game::SceneSerializer::from_json(
      restored.scenes(),
      json,
      make_scene);

  if (!restored_result)
  {
    vix::print("failed to restore scenes:", restored_result.error().message());
    return 1;
  }

  vix::print("restored scene count:", restored.scenes().size());
  vix::print("restored active scene:", restored.scenes().active_name());

  return 0;
}
```

## Assets

`AssetManager` loads files from an asset root.

Create:

```txt
assets/example.txt
```

Example content:

```txt
Hello from a Vix game asset.
```

Then load it:

```cpp
#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::AssetManager assets("assets");

  auto loaded = assets.load("example.txt");
  if (!loaded)
  {
    vix::print("failed to load asset:", loaded.error().message());
    return 1;
  }

  auto *asset = assets.get(loaded.value());
  if (asset == nullptr)
  {
    vix::print("asset not found after loading");
    return 1;
  }

  vix::print("asset loaded");
  vix::print("id:", asset->id());
  vix::print("path:", asset->path().relative());
  vix::print("size:", asset->size());

  if (vix::game::is_text_asset(asset->type()))
  {
    vix::print("content:", asset->text_content());
  }

  return 0;
}
```

Use `AssetManager` for:

- text files
- binary files
- images
- level files
- metadata
- future asset pipeline workflows

## Async asset loading

Use `AsyncAssetLoader` when you want to load assets in the background.

```cpp
#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::AssetManager assets("assets");
  vix::game::JobSystem jobs;
  vix::game::AsyncAssetLoader loader(assets, jobs);

  auto handle = loader.load(
      "example.txt",
      [](vix::game::GameResult<vix::game::AssetId> result)
      {
        if (!result)
        {
          vix::print("async asset failed:", result.error().message());
          return;
        }

        vix::print("async asset loaded:", result.value());
      });

  if (!handle)
  {
    vix::print("failed to start async load:", handle.error().message());
    return 1;
  }

  handle.value().wait();
  handle.value().get();

  jobs.shutdown();

  return 0;
}
```

Use this when asset loading should not block the main thread.

## Jobs

`JobSystem` runs work in the background.

```cpp
#include <vix/game/JobSystem.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::JobSystem jobs;

  auto submitted = jobs.submit(
      []
      {
        vix::print("job executed");
      });

  if (!submitted)
  {
    vix::print("failed to submit job:", submitted.error().message());
    return 1;
  }

  submitted.value().wait();
  submitted.value().get();

  auto detached = jobs.submit_detached(
      []
      {
        vix::print("detached job executed");
      });

  if (!detached)
  {
    vix::print("failed to submit detached job:", detached.error().message());
    return 1;
  }

  jobs.shutdown();

  return 0;
}
```

Use jobs for:

- asset loading
- heavy calculations
- scene preparation
- background processing
- future editor tasks

## Input system

`InputSystem` gives you backend-independent input state.

It can track:

- keyboard keys
- mouse buttons
- pointer position
- action bindings
- pressed, down, and released states

```cpp
#include <vix/game/InputSystem.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::InputSystem input;

  auto jump = input.bind_key("jump", vix::game::InputKey::Space);
  if (!jump)
  {
    vix::print("failed to bind jump:", jump.error().message());
    return 1;
  }

  auto shoot = input.bind_button("shoot", vix::game::InputButton::Left);
  if (!shoot)
  {
    vix::print("failed to bind shoot:", shoot.error().message());
    return 1;
  }

  input.begin_frame();
  input.press_key(vix::game::InputKey::Space);
  input.press_button(vix::game::InputButton::Left);

  vix::print("space down:", input.key_down(vix::game::InputKey::Space));
  vix::print("jump pressed:", input.action_pressed("jump"));
  vix::print("shoot pressed:", input.action_pressed("shoot"));

  input.begin_frame();

  vix::print("jump down:", input.action_down("jump"));
  vix::print("jump pressed next frame:", input.action_pressed("jump"));

  input.release_key(vix::game::InputKey::Space);

  vix::print("jump released:", input.action_released("jump"));

  return 0;
}
```

Use action names such as:

```txt
jump
shoot
move_left
move_right
pause
interact
```

This avoids spreading raw keys everywhere in your game logic.

## Window abstraction

`Window` is backend-independent.

You can use a null backend for tests, CI, or headless simulations.

```cpp
#include <memory>
#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::Window window(std::make_unique<vix::game::NullWindow>());

  auto opened = window.open(vix::game::WindowConfig{
      .title = "Null Window",
      .width = 800,
      .height = 600,
      .fullscreen = false,
      .resizable = true,
      .visible = false,
      .vsync = false,
      .headless = true});

  if (!opened)
  {
    vix::print("failed to open window:", opened.error().message());
    return 1;
  }

  vix::print("window backend:", window.backend_name());
  vix::print("window title:", window.title());
  vix::print("window size:", window.width(), "x", window.height());

  window.resize(1024, 768);

  for (const auto &event : window.poll_events())
  {
    vix::print("event:", vix::game::to_string(event.type));
  }

  window.close();

  return 0;
}
```

Use `NullWindow` when you want to test window logic without opening a real OS window.

## Renderer abstraction

`Renderer` is also backend-independent.

A null renderer is useful for testing rendering flow without a GPU backend.

```cpp
#include <memory>
#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::Window window(std::make_unique<vix::game::NullWindow>());

  auto opened = window.open(vix::game::WindowConfig::headless_config());
  if (!opened)
  {
    vix::print("failed to open window:", opened.error().message());
    return 1;
  }

  vix::game::Renderer renderer(std::make_unique<vix::game::NullRenderer>());

  auto initialized = renderer.init(window);
  if (!initialized)
  {
    vix::print("failed to init renderer:", initialized.error().message());
    return 1;
  }

  renderer.begin_frame();
  renderer.clear(vix::game::Color::from_rgba8(20, 20, 30));
  renderer.end_frame();

  vix::print("renderer backend:", renderer.backend_name());
  vix::print("renderer size:", renderer.width(), "x", renderer.height());

  renderer.shutdown();
  window.close();

  return 0;
}
```

## Renderer2D

`Renderer2D` provides a 2D drawing layer on top of the renderer.

```cpp
#include <memory>
#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::Window window(std::make_unique<vix::game::NullWindow>());

  auto opened = window.open(vix::game::WindowConfig::headless_config());
  if (!opened)
  {
    vix::print("failed to open window:", opened.error().message());
    return 1;
  }

  vix::game::Renderer renderer(std::make_unique<vix::game::NullRenderer>());

  auto initialized = renderer.init(window);
  if (!initialized)
  {
    vix::print("failed to initialize renderer:", initialized.error().message());
    return 1;
  }

  vix::game::Renderer2D renderer2d(renderer);

  vix::game::Transform2D transform;
  transform.set_position(vix::game::Vec2{100.0F, 80.0F});
  transform.set_scale(2.0F);

  vix::game::Sprite sprite;
  sprite.set_texture(vix::game::TextureHandle::from_asset(1));
  sprite.set_size(vix::game::Vec2{64.0F, 64.0F});
  sprite.set_tint(vix::game::Color::white());

  renderer2d.begin_frame();
  renderer2d.clear(vix::game::Color::black());
  renderer2d.draw_sprite(transform, sprite);
  renderer2d.end_frame();

  vix::print("2D commands:", renderer2d.command_count());

  renderer.shutdown();
  window.close();

  return 0;
}
```

This example records 2D commands with a null renderer.

For real drawing, use a real renderer backend such as SDL/OpenGL.

## Runtime layers

`vix/game` also provides runtime layers for future growth:

- `GameRuntime`
- `EditorRuntime`
- `ScriptRuntime`
- `AudioRuntime`
- `PhysicsRuntime`

Example:

```cpp
#include <vix/game/all.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::App app;

  vix::game::GameRuntime runtime(app);
  auto init = runtime.init();
  if (!init)
  {
    vix::print("runtime init failed:", init.error().message());
    return 1;
  }

  vix::game::EditorRuntime editor(runtime);
  auto editor_init = editor.init();
  if (!editor_init)
  {
    vix::print("editor init failed:", editor_init.error().message());
    return 1;
  }

  vix::game::ScriptRuntime scripts(runtime.context());
  auto scripts_init = scripts.init();
  if (!scripts_init)
  {
    vix::print("script runtime init failed:", scripts_init.error().message());
    return 1;
  }

  vix::game::AudioRuntime audio(runtime.context());
  auto audio_init = audio.init();
  if (!audio_init)
  {
    vix::print("audio runtime init failed:", audio_init.error().message());
    return 1;
  }

  vix::game::PhysicsRuntime physics(runtime.context());
  auto physics_init = physics.init();
  if (!physics_init)
  {
    vix::print("physics runtime init failed:", physics_init.error().message());
    return 1;
  }

  auto package = vix::game::GamePackage::defaults();

  vix::print("V3 runtime ready");
  vix::print("package:", package.name);
  vix::print("entry scene:", package.entry_scene);

  editor.enter_play_mode();
  vix::print("editor play mode:", editor.context().play_mode());

  runtime.shutdown();

  return 0;
}
```

These runtime layers are foundations.

They do not force you to use a full editor, audio engine, scripting engine, or physics engine immediately.

## SDL window with null renderer

When you want a real OS window but no real renderer yet, use SDL window + null renderer.

```cpp
#include <memory>

#include <vix/game/App.hpp>
#include <vix/game/AppConfig.hpp>
#include <vix/game/Frame.hpp>
#include <vix/game/InputKey.hpp>
#include <vix/game/NullRenderer.hpp>
#include <vix/game/Renderer2D.hpp>
#include <vix/game/Scene.hpp>
#include <vix/game/Sprite.hpp>
#include <vix/game/Transform2D.hpp>
#include <vix/game/Vec2.hpp>
#include <vix/game/WindowConfig.hpp>

#include <vix/game/backends/sdl/SDLWindow.hpp>

class SDLWindowDemoScene final : public vix::game::Scene
{
public:
  void on_update(const vix::game::Frame &frame) override
  {
    (void)frame;

    if (input().key_pressed(vix::game::InputKey::Escape))
    {
      app().stop();
      return;
    }

    auto pointer = input().pointer_position();
    auto delta = input().pointer_delta();

    if (delta.x != 0.0 || delta.y != 0.0)
    {
      (void)pointer;
    }

    vix::game::Sprite sprite = vix::game::Sprite::from_asset(1);
    sprite.set_size(vix::game::Vec2{64.0F, 64.0F});

    vix::game::Transform2D transform =
        vix::game::Transform2D::at(vix::game::Vec2{100.0F, 100.0F});

    renderer2d().draw_sprite(transform, sprite);
  }
};

int main()
{
  vix::game::AppConfig config = vix::game::AppConfig::defaults();
  config.headless = false;
  config.title = "Vix Game SDL Window + NullRenderer";
  config.window = vix::game::WindowConfig::defaults();
  config.window.title = config.title;
  config.window.width = 960;
  config.window.height = 540;
  config.window.resizable = true;
  config.window.visible = true;
  config.window.vsync = true;
  config.window.headless = false;

  vix::game::App app(config);

  auto init = app.init();
  if (!init)
  {
    return 1;
  }

  auto &context = app.runtime().context();

  context
      .set_window_backend(std::make_unique<vix::game::sdl::SDLWindow>())
      .set_renderer_backend(std::make_unique<vix::game::NullRenderer>());

  auto opened = context.window().open(config.window);
  if (!opened)
  {
    return 1;
  }

  auto renderer_init = context.renderer().init(context.window());
  if (!renderer_init)
  {
    return 1;
  }

  context.renderer2d().set_camera(
      vix::game::Camera2D::from_size(
          context.window().width(),
          context.window().height()));

  auto scene = app.scenes().create<SDLWindowDemoScene>("main");
  if (!scene)
  {
    return 1;
  }

  auto active = app.scenes().set_active("main");
  if (!active)
  {
    return 1;
  }

  auto run = app.run();
  if (!run)
  {
    return 1;
  }

  app.shutdown();
  return 0;
}
```

## SDL renderer

Use SDL renderer when you want a real window and a real renderer backend.

```cpp
#include <memory>

#include <vix/game/App.hpp>
#include <vix/game/AppConfig.hpp>
#include <vix/game/Color.hpp>
#include <vix/game/Frame.hpp>
#include <vix/game/InputKey.hpp>
#include <vix/game/Scene.hpp>
#include <vix/game/WindowConfig.hpp>

#include <vix/game/backends/sdl/SDLRenderer.hpp>
#include <vix/game/backends/sdl/SDLWindow.hpp>

class SDLRendererDemoScene final : public vix::game::Scene
{
public:
  void on_update(const vix::game::Frame &frame) override
  {
    if (input().key_pressed(vix::game::InputKey::Escape))
    {
      app().stop();
      return;
    }

    const float t = static_cast<float>((frame.index % 255) / 255.0);

    renderer2d().clear(vix::game::Color{0.08F, t * 0.25F, 0.18F, 1.0F});
  }
};

int main()
{
  vix::game::AppConfig config = vix::game::AppConfig::defaults();
  config.headless = false;
  config.title = "Vix Game SDLRenderer Demo";

  config.window = vix::game::WindowConfig::defaults();
  config.window.title = config.title;
  config.window.width = 960;
  config.window.height = 540;
  config.window.resizable = true;
  config.window.visible = true;
  config.window.vsync = true;
  config.window.headless = false;

  vix::game::App app(config);

  auto init = app.init();
  if (!init)
  {
    return 1;
  }

  auto &context = app.runtime().context();

  context
      .set_window_backend(std::make_unique<vix::game::sdl::SDLWindow>())
      .set_renderer_backend(std::make_unique<vix::game::sdl::SDLRenderer>());

  auto opened = context.window().open(config.window);
  if (!opened)
  {
    return 1;
  }

  auto renderer_init = context.renderer().init(context.window());
  if (!renderer_init)
  {
    return 1;
  }

  auto scene = app.scenes().create<SDLRendererDemoScene>("main");
  if (!scene)
  {
    return 1;
  }

  auto active = app.scenes().set_active("main");
  if (!active)
  {
    return 1;
  }

  auto run = app.run();
  if (!run)
  {
    return 1;
  }

  app.shutdown();
  return 0;
}
```

## SDL OpenGL renderer

Use SDL OpenGL when you want an SDL window with an OpenGL renderer backend.

```cpp
#include <memory>

#include <vix/game/all.hpp>
#include <vix/game/backends/sdl/SDLOpenGLRenderer.hpp>
#include <vix/game/backends/sdl/SDLWindow.hpp>
#include <vix/print.hpp>

class OpenGLDemoScene final : public vix::game::Scene
{
public:
  vix::game::GameBoolResult on_load() override
  {
    vix::print("SDL OpenGL scene loaded");
    return vix::game::Scene::on_load();
  }

  void on_update(const vix::game::Frame &frame) override
  {
    if (input().key_pressed(vix::game::InputKey::Escape))
    {
      app().stop();
      return;
    }

    renderer2d().clear(vix::game::Color{0.05F, 0.06F, 0.08F, 1.0F});

    if (frame.index % 60 == 0)
    {
      vix::print("SDL OpenGL frame:", frame.index);
    }
  }
};

int main()
{
  vix::game::AppConfig config = vix::game::AppConfig::defaults();
  config.headless = false;
  config.title = "Vix Game SDL OpenGL Demo";

  config.window = vix::game::WindowConfig::defaults();
  config.window.title = config.title;
  config.window.width = 960;
  config.window.height = 540;
  config.window.resizable = true;
  config.window.visible = true;
  config.window.vsync = true;
  config.window.headless = false;
  config.window.opengl = true;

  vix::game::App app(config);

  auto init = app.init();
  if (!init)
  {
    vix::print("app init failed:", init.error().message());
    return 1;
  }

  auto &context = app.runtime().context();

  context
      .set_window_backend(std::make_unique<vix::game::sdl::SDLWindow>())
      .set_renderer_backend(std::make_unique<vix::game::sdl::SDLOpenGLRenderer>());

  auto opened = context.window().open(config.window);
  if (!opened)
  {
    vix::print("window open failed:", opened.error().message());
    return 1;
  }

  auto renderer_init = context.renderer().init(context.window());
  if (!renderer_init)
  {
    vix::print("renderer init failed:", renderer_init.error().message());
    return 1;
  }

  context.renderer2d().set_camera(
      vix::game::Camera2D::from_size(
          context.window().width(),
          context.window().height()));

  auto scene = app.scenes().create<OpenGLDemoScene>("main");
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

  auto run = app.run();
  if (!run)
  {
    vix::print("game failed:", run.error().message());
    return 1;
  }

  app.shutdown();
  return 0;
}
```

## Sprite rendering

Sprite rendering combines:

```txt
AssetManager
Renderer texture upload
Sprite
Transform2D
Renderer2D
```

The flow is:

```txt
load image asset
  -> upload texture to renderer
      -> create sprite from asset id
          -> draw sprite with transform
```

A sprite scene usually does this:

```cpp
auto loaded = assets().load_or_get("player.jpg");
texture_id_ = loaded.value();

auto *asset = assets().get(texture_id_);
auto uploaded = renderer().upload_texture(*asset);

vix::game::Sprite sprite = vix::game::Sprite::from_asset(texture_id_);
sprite.set_size(vix::game::Vec2{128.0F, 128.0F});

renderer2d().draw_sprite(transform, sprite);
```

Use the dedicated sprite example when you need a full texture rendering demo.

## Tiny Adventure example

A small interactive game combines:

- SDL window
- OpenGL renderer
- asset loading
- texture upload
- keyboard movement
- collision detection
- sprite rendering
- scoring

The example flow is:

```txt
load player texture
load coin texture
move player with WASD or arrows
detect player/coin collision
draw player
draw coin
stop with Escape
```

Core scene responsibilities:

```txt
on_load()
  -> load assets
  -> upload textures

on_update(frame)
  -> handle Escape
  -> update player
  -> update coin
  -> render frame
```

The full example can live in your examples folder, while this guide teaches the structure.

## Export a game project

Use `GameExporter` from C++:

```cpp
#include <vix/game/all.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::GameExportConfig config;
  config.project_root = ".";
  config.output_directory = "dist";
  config.name = "demo-game";

  vix::game::GameExporter exporter;

  auto result = exporter.export_project(config);
  if (!result)
  {
    vix::print("export failed:", result.error().message());
    return 1;
  }

  vix::print("exported to:", result.value().output_path.string());
  vix::print("copied files:", result.value().copied_files);
  vix::print("copied directories:", result.value().copied_directories);

  return 0;
}
```

Or use the CLI:

```bash
vix game export
```

Expected output layout:

```txt
dist/
  mario/
    assets/
    game.package.json
    README.md
    export.json
```

## Install SDL/OpenGL dependencies

For SDL/OpenGL examples, install system dependencies.

Ubuntu or Debian:

```bash
sudo apt update
sudo apt install -y libsdl2-dev libsdl2-image-dev libgl1-mesa-dev
```

Then build with SDL support if required by your project or module configuration.

For module-level examples, this kind of build is commonly used:

```bash
vix build --build-target all -v -- -DVIX_GAME_BUILD_TESTS=ON -DVIX_GAME_BUILD_EXAMPLES=ON -DVIX_GAME_ENABLE_SDL=ON
```

## Suggested learning order

Follow this order:

```txt
1. Hello game app
2. Basic game loop
3. Scenes
4. Assets
5. Jobs
6. Async assets
7. Input
8. Window abstraction
9. Renderer abstraction
10. Renderer2D
11. SDL window
12. SDL renderer
13. SDL OpenGL renderer
14. Sprite rendering
15. Game export
```

This avoids jumping directly into SDL/OpenGL before understanding the runtime.

## How to grow a game project

The generated project starts with:

```txt
src/main.cpp
```

When the project grows, split it:

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
├── assets/
│   └── AssetIds.hpp
└── gameplay/
    ├── Player.hpp
    └── Coin.hpp
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

## Common mistakes

### Expecting a full engine immediately

`vix/game` is a foundation.

It gives you runtime structure.

You build your game architecture on top.

### Starting with SDL/OpenGL too early

Learn the runtime first.

Then move to window and renderer backends.

### Forgetting to update `vix.app`

When you add `.cpp` files, add them to:

```txt
sources = [
]
```

### Forgetting to shut down jobs

If you create a `JobSystem`, shut it down when done:

```cpp
jobs.shutdown();
```

### Putting assets outside `assets/`

Keep assets under:

```txt
assets/
```

This keeps export and packaging simple.

### Loading images without uploading textures

For sprite rendering, loading an image asset is not enough.

You also need:

```cpp
renderer().upload_texture(*asset);
```

## What you should remember

`vix/game` gives you the foundation:

```txt
App
GameRuntime
Scene
Frame
AssetManager
JobSystem
InputSystem
Window
Renderer
Renderer2D
GameExporter
```

The clean progression is:

```txt
runtime
  -> scenes
      -> assets
          -> input
              -> renderer
                  -> export
```

Create a game:

```bash
vix new mario --game
cd mario
vix build
vix run
```

Export it:

```bash
vix game export
```

For SDL/OpenGL:

```bash
sudo apt install -y libsdl2-dev libsdl2-image-dev libgl1-mesa-dev
```

## Next steps

Continue with:

- [Game template](/templates/game)
- [Application template](/templates/application)
- [vix.app](/guides/vix-app/)
- [Build and run](/cli/run)
