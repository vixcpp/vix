#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::App app;

  app.set_title("Hello Vix Game");
  app.set_target_fps(60);

  vix::print("Hello from vix/game");
  vix::print("title:", app.config().title);

  return 0;
}
