#include <vix/game/InputSystem.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::InputSystem input;

  auto jump = input.bind_key("jump", vix::game::InputKey::Space);
  if (!jump)
  {
    vix::print("failed to bind jump:", jump.error().message());
    return 1;
  }

  auto shoot = input.bind_button("shoot", vix::game::InputButton::Left);
  if (!shoot)
  {
    vix::print("failed to bind shoot:", shoot.error().message());
    return 1;
  }

  input.begin_frame();
  input.press_key(vix::game::InputKey::Space);
  input.press_button(vix::game::InputButton::Left);

  vix::print("space down:", input.key_down(vix::game::InputKey::Space));
  vix::print("jump pressed:", input.action_pressed("jump"));
  vix::print("shoot pressed:", input.action_pressed("shoot"));

  input.begin_frame();

  vix::print("jump down:", input.action_down("jump"));
  vix::print("jump pressed next frame:", input.action_pressed("jump"));

  input.release_key(vix::game::InputKey::Space);

  vix::print("jump released:", input.action_released("jump"));

  return 0;
}
