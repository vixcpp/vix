/**
 *
 *  @file sprite_demo.cpp
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
#include <vix/game/Sprite.hpp>
#include <vix/game/Transform2D.hpp>
#include <vix/game/Vec2.hpp>
#include <vix/game/WindowConfig.hpp>

#include <vix/game/backends/sdl/SDLOpenGLRenderer.hpp>
#include <vix/game/backends/sdl/SDLWindow.hpp>

class SpriteDemoScene final : public vix::game::Scene
{
public:
  vix::game::GameBoolResult on_load() override
  {
    auto loaded = assets().load_or_get("player.jpg");
    if (!loaded)
    {
      return loaded.error();
    }

    texture_id_ = loaded.value();

    auto *asset = assets().get(texture_id_);
    if (asset == nullptr)
    {
      return vix::game::make_game_error(
          vix::game::GameErrorCode::AssetNotFound,
          "sprite demo asset not found after loading");
    }

    auto uploaded = renderer().upload_texture(*asset);
    if (!uploaded)
    {
      return uploaded.error();
    }

    return vix::game::Scene::on_load();
  }

  void on_update(const vix::game::Frame &frame) override
  {
    if (input().key_pressed(vix::game::InputKey::Escape))
    {
      app().stop();
      return;
    }

    renderer2d().clear(vix::game::Color{0.06F, 0.07F, 0.09F, 1.0F});

    const float x = 320.0F;
    const float y = 180.0F;

    vix::game::Transform2D transform =
        vix::game::Transform2D::at(vix::game::Vec2{x, y});

    transform.rotation =
        static_cast<float>(frame.index % 360) * 0.017453292519943295F;

    transform.origin = vix::game::Vec2{64.0F, 64.0F};

    vix::game::Sprite sprite = vix::game::Sprite::from_asset(texture_id_);
    sprite.set_size(vix::game::Vec2{128.0F, 128.0F});

    renderer2d().draw_sprite(transform, sprite);
  }

private:
  vix::game::AssetId texture_id_{vix::game::invalid_asset_id};
};

int main()
{
  vix::game::AppConfig config = vix::game::AppConfig::defaults();
  config.headless = false;
  config.title = "Vix Game Sprite Demo";
  config.asset_root = "examples/assets";

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
    return 1;
  }

  auto &context = app.runtime().context();

  context
      .set_window_backend(std::make_unique<vix::game::sdl::SDLWindow>())
      .set_renderer_backend(std::make_unique<vix::game::sdl::SDLOpenGLRenderer>());

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

  auto scene = app.scenes().create<SpriteDemoScene>("main");
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
