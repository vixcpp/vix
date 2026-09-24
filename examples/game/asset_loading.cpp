#include <vix/game/game.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::AssetManager assets("assets");

  auto loaded = assets.load("example.txt");
  if (!loaded)
  {
    vix::print("failed to load asset:", loaded.error().message());
    return 1;
  }

  auto *asset = assets.get(loaded.value());
  if (asset == nullptr)
  {
    vix::print("asset not found after loading");
    return 1;
  }

  vix::print("asset loaded");
  vix::print("id:", asset->id());
  vix::print("path:", asset->path().relative());
  vix::print("size:", asset->size());

  if (vix::game::is_text_asset(asset->type()))
  {
    vix::print("content:", asset->text_content());
  }

  return 0;
}
