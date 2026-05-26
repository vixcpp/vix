#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::AssetManager assets("assets");
  vix::game::JobSystem jobs;
  vix::game::AsyncAssetLoader loader(assets, jobs);

  auto handle = loader.load(
      "example.txt",
      [](vix::game::GameResult<vix::game::AssetId> result)
      {
        if (!result)
        {
          vix::print("async asset failed:", result.error().message());
          return;
        }

        vix::print("async asset loaded:", result.value());
      });

  if (!handle)
  {
    vix::print("failed to start async load:", handle.error().message());
    return 1;
  }

  handle.value().wait();
  handle.value().get();

  jobs.shutdown();

  return 0;
}
