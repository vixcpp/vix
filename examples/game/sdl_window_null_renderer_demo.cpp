/**
 *
 *  @file sdl_window_null_renderer_demo.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp
 *
 */

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
      // The demo intentionally avoids logging every frame.
      // Pointer state is updated through SDL events and InputSystem.
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
