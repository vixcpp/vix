/**
 *
 *  @file jobs_demo.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/game
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix.cpp
 *
 */

#include <vix/game/game.hpp>
#include <vix/print.hpp>

class MainScene final : public vix::game::Scene
{
public:
  MainScene()
      : vix::game::Scene("main")
  {
  }

  vix::game::GameBoolResult on_load() override
  {
    vix::print("main scene loaded");
    return vix::game::Scene::on_load();
  }

  void on_enter() override
  {
    vix::game::Scene::on_enter();
    vix::print("main scene entered");
  }

  void on_update(const vix::game::Frame &frame) override
  {
    vix::print("scene frame:", frame.index);

    if (frame.index >= 5)
    {
      app().stop();
    }
  }

  vix::game::GameBoolResult on_unload() override
  {
    vix::print("main scene unloaded");
    return vix::game::Scene::on_unload();
  }
};

int main()
{
  vix::game::App app;

  auto added = app.scenes().create<MainScene>("main");
  if (!added)
  {
    vix::print("error:", added.error().message());
    return 1;
  }

  auto active = app.scenes().set_active("main");
  if (!active)
  {
    vix::print("error:", active.error().message());
    return 1;
  }

  auto result = app.run();
  if (!result)
  {
    vix::print("error:", result.error().message());
    return 1;
  }

  return 0;
}
