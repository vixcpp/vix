#include <vix/game/all.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::GameExportConfig config;
  config.project_root = ".";
  config.output_directory = "dist";
  config.name = "demo-game";

  vix::game::GameExporter exporter;

  auto result = exporter.export_project(config);
  if (!result)
  {
    vix::print("export failed:", result.error().message());
    return 1;
  }

  vix::print("exported to:", result.value().output_path.string());
  vix::print("copied files:", result.value().copied_files);
  vix::print("copied directories:", result.value().copied_directories);

  return 0;
}
