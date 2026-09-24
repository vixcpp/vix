/**
 *
 *  @file opengl_renderer_demo.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp
 *
 */

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
