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
