#include <vix/game/game.hpp>
#include <vix/print.hpp>

class MainScene final : public vix::game::Scene
{
public:
  MainScene()
      : vix::game::Scene("main")
  {
  }
};

class MenuScene final : public vix::game::Scene
{
public:
  MenuScene()
      : vix::game::Scene("menu")
  {
  }
};

static std::unique_ptr<vix::game::Scene> make_scene(
    const std::string &name)
{
  if (name == "main")
  {
    return std::make_unique<MainScene>();
  }

  if (name == "menu")
  {
    return std::make_unique<MenuScene>();
  }

  return nullptr;
}

int main()
{
  vix::game::App app;

  auto menu = app.scenes().create<MenuScene>("menu");
  if (!menu)
  {
    vix::print("failed to create menu scene:", menu.error().message());
    return 1;
  }

  auto main = app.scenes().create<MainScene>("main");
  if (!main)
  {
    vix::print("failed to create main scene:", main.error().message());
    return 1;
  }

  auto active = app.scenes().set_active("main");
  if (!active)
  {
    vix::print("failed to activate scene:", active.error().message());
    return 1;
  }

  auto json = vix::game::SceneSerializer::to_json(app.scenes());

  vix::print("serialized scene count:", app.scenes().size());
  vix::print("active scene:", app.scenes().active_name());

  vix::game::App restored;

  auto restored_result = vix::game::SceneSerializer::from_json(
      restored.scenes(),
      json,
      make_scene);

  if (!restored_result)
  {
    vix::print("failed to restore scenes:", restored_result.error().message());
    return 1;
  }

  vix::print("restored scene count:", restored.scenes().size());
  vix::print("restored active scene:", restored.scenes().active_name());

  return 0;
}
