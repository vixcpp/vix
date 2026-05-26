/**
 *
 *  @file window_input_demo.cpp
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
#include <vix/game/Scene.hpp>
#include <vix/game/WindowConfig.hpp>

#include <vix/game/backends/sdl/SDLWindow.hpp>

#include <vix/print.hpp>

class WindowInputDemoScene final : public vix::game::Scene
{
public:
  void on_update(const vix::game::Frame &frame) override
  {
    (void)frame;

    auto &ctx = app().runtime().context();

    auto pointer = ctx.input().pointer_position();
    auto pointer_delta = ctx.input().pointer_delta();

    if (ctx.input().key_pressed(vix::game::InputKey::Escape))
    {
      vix::print("Escape pressed, closing app");
      app().stop();
      return;
    }

    if (pointer_delta.x != 0.0 || pointer_delta.y != 0.0)
    {
      vix::print(
          "pointer:",
          pointer.x,
          pointer.y,
          "delta:",
          pointer_delta.x,
          pointer_delta.y);
    }

    const auto width = ctx.window().width();
    const auto height = ctx.window().height();

    if (width != last_width_ || height != last_height_)
    {
      last_width_ = width;
      last_height_ = height;

      vix::print("window resized:", width, "x", height);
    }
  }

private:
  std::uint32_t last_width_{0};
  std::uint32_t last_height_{0};
};

int main()
{
  vix::game::AppConfig config = vix::game::AppConfig::defaults();
  config.headless = false;
  config.title = "Vix Game Window/Input Demo";

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

  auto scene = app.scenes().create<WindowInputDemoScene>("main");
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
