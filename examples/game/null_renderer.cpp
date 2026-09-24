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
