#include <vix/game/all.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::App app;

  vix::game::GameRuntime runtime(app);
  auto init = runtime.init();
  if (!init)
  {
    vix::print("runtime init failed:", init.error().message());
    return 1;
  }

  vix::game::EditorRuntime editor(runtime);
  auto editor_init = editor.init();
  if (!editor_init)
  {
    vix::print("editor init failed:", editor_init.error().message());
    return 1;
  }

  vix::game::ScriptRuntime scripts(runtime.context());
  auto scripts_init = scripts.init();
  if (!scripts_init)
  {
    vix::print("script runtime init failed:", scripts_init.error().message());
    return 1;
  }

  vix::game::AudioRuntime audio(runtime.context());
  auto audio_init = audio.init();
  if (!audio_init)
  {
    vix::print("audio runtime init failed:", audio_init.error().message());
    return 1;
  }

  vix::game::PhysicsRuntime physics(runtime.context());
  auto physics_init = physics.init();
  if (!physics_init)
  {
    vix::print("physics runtime init failed:", physics_init.error().message());
    return 1;
  }

  auto package = vix::game::GamePackage::defaults();

  vix::print("V3 runtime ready");
  vix::print("package:", package.name);
  vix::print("entry scene:", package.entry_scene);

  editor.enter_play_mode();
  vix::print("editor play mode:", editor.context().play_mode());

  runtime.shutdown();

  return 0;
}
