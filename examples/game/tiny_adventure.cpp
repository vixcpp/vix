/**
 *
 *  @file tiny_adventure.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp
 *
 */

#include <cmath>
#include <memory>

#include <vix/game/App.hpp>
#include <vix/game/AppConfig.hpp>
#include <vix/game/Camera2D.hpp>
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
#include <vix/print.hpp>

namespace
{
  struct Player
  {
    vix::game::Vec2 position{420.0F, 260.0F};
    vix::game::Vec2 size{96.0F, 96.0F};
    float speed{260.0F};
  };

  struct Coin
  {
    vix::game::Vec2 position{680.0F, 260.0F};
    vix::game::Vec2 size{64.0F, 64.0F};
    bool collected{false};
  };

  bool intersects(
      const vix::game::Vec2 &a_position,
      const vix::game::Vec2 &a_size,
      const vix::game::Vec2 &b_position,
      const vix::game::Vec2 &b_size)
  {
    const float a_left = a_position.x;
    const float a_right = a_position.x + a_size.x;
    const float a_top = a_position.y;
    const float a_bottom = a_position.y + a_size.y;

    const float b_left = b_position.x;
    const float b_right = b_position.x + b_size.x;
    const float b_top = b_position.y;
    const float b_bottom = b_position.y + b_size.y;

    return a_left < b_right &&
           a_right > b_left &&
           a_top < b_bottom &&
           a_bottom > b_top;
  }

  float clamp_float(
      float value,
      float minimum,
      float maximum)
  {
    if (value < minimum)
    {
      return minimum;
    }

    if (value > maximum)
    {
      return maximum;
    }

    return value;
  }
}

class TinyAdventureScene final : public vix::game::Scene
{
public:
  vix::game::GameBoolResult on_load() override
  {
    vix::print("Tiny Adventure loaded");
    vix::print("Move with arrow keys or WASD. Press Escape to quit.");

    auto player_asset = assets().load_or_get("player.jpg");
    if (!player_asset)
    {
      vix::print("failed to load player.jpg:", player_asset.error().message());
      return player_asset.error();
    }

    player_texture_ = player_asset.value();

    auto *player_image = assets().get(player_texture_);
    if (player_image == nullptr)
    {
      return vix::game::make_game_error(
          vix::game::GameErrorCode::AssetNotFound,
          "player.jpg was loaded but cannot be found");
    }

    auto player_upload = renderer().upload_texture(*player_image);
    if (!player_upload)
    {
      vix::print("failed to upload player texture:", player_upload.error().message());
      return player_upload.error();
    }

    auto coin_asset = assets().load_or_get("coin.jpeg");
    if (!coin_asset)
    {
      vix::print("failed to load coin.jpg:", coin_asset.error().message());
      return coin_asset.error();
    }

    coin_texture_ = coin_asset.value();

    auto *coin_image = assets().get(coin_texture_);
    if (coin_image == nullptr)
    {
      return vix::game::make_game_error(
          vix::game::GameErrorCode::AssetNotFound,
          "coin.jpg was loaded but cannot be found");
    }

    auto coin_upload = renderer().upload_texture(*coin_image);
    if (!coin_upload)
    {
      vix::print("failed to upload coin texture:", coin_upload.error().message());
      return coin_upload.error();
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

    update_player(frame);
    update_coin();
    render(frame);
  }

private:
  void update_player(const vix::game::Frame &frame)
  {
    vix::game::Vec2 direction{0.0F, 0.0F};

    if (input().key_down(vix::game::InputKey::A) ||
        input().key_down(vix::game::InputKey::ArrowLeft))
    {
      direction.x -= 1.0F;
    }

    if (input().key_down(vix::game::InputKey::D) ||
        input().key_down(vix::game::InputKey::ArrowRight))
    {
      direction.x += 1.0F;
    }

    if (input().key_down(vix::game::InputKey::W) ||
        input().key_down(vix::game::InputKey::ArrowUp))
    {
      direction.y -= 1.0F;
    }

    if (input().key_down(vix::game::InputKey::S) ||
        input().key_down(vix::game::InputKey::ArrowDown))
    {
      direction.y += 1.0F;
    }

    const float length =
        std::sqrt(direction.x * direction.x + direction.y * direction.y);

    if (length > 0.0F)
    {
      direction.x /= length;
      direction.y /= length;
    }

    const float delta_seconds =
        static_cast<float>(frame.delta_ms()) / 1000.0F;

    player_.position.x += direction.x * player_.speed * delta_seconds;
    player_.position.y += direction.y * player_.speed * delta_seconds;

    player_.position.x = clamp_float(player_.position.x, 0.0F, 960.0F - player_.size.x);
    player_.position.y = clamp_float(player_.position.y, 0.0F, 540.0F - player_.size.y);
  }

  void update_coin()
  {
    if (coin_.collected)
    {
      return;
    }

    if (intersects(
            player_.position,
            player_.size,
            coin_.position,
            coin_.size))
    {
      coin_.collected = true;
      ++score_;

      vix::print("coin collected. score:", score_);
    }
  }

  void render(const vix::game::Frame &frame)
  {
    const float pulse =
        0.5F + 0.5F * std::sin(static_cast<float>(frame.index) * 0.04F);

    renderer2d().clear(vix::game::Color{0.05F, 0.06F, 0.09F, 1.0F});

    draw_player();

    if (!coin_.collected)
    {
      draw_coin(pulse);
    }
  }

  void draw_player()
  {
    vix::game::Transform2D transform =
        vix::game::Transform2D::at(player_.position);

    vix::game::Sprite sprite = vix::game::Sprite::from_asset(player_texture_);
    sprite.set_size(player_.size);

    renderer2d().draw_sprite(transform, sprite);
  }

  void draw_coin(float pulse)
  {
    vix::game::Transform2D transform =
        vix::game::Transform2D::at(coin_.position);

    const float scale = 1.0F + pulse * 0.15F;
    transform.scale = vix::game::Vec2{scale, scale};

    vix::game::Sprite sprite = vix::game::Sprite::from_asset(coin_texture_);
    sprite.set_size(coin_.size);
    sprite.tint = vix::game::Color{1.0F, 0.9F, 0.35F, 1.0F};

    renderer2d().draw_sprite(transform, sprite);
  }

private:
  Player player_{};
  Coin coin_{};

  int score_{0};

  vix::game::AssetId player_texture_{vix::game::invalid_asset_id};
  vix::game::AssetId coin_texture_{vix::game::invalid_asset_id};
};

int main()
{
  vix::game::AppConfig config = vix::game::AppConfig::defaults();
  config.headless = false;
  config.title = "Vix Game Tiny Adventure";
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

  auto scene = app.scenes().create<TinyAdventureScene>("main");
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
