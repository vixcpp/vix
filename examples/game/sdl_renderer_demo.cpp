/**
 *
 *  @file sdl_renderer_demo.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp
 *
 */

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
