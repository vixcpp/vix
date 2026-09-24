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
