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
