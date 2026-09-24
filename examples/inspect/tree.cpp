#include <vix/inspect.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

struct DatabaseConfig
{
  std::string host;
  int port;
  bool ssl;
};

struct AppConfig
{
  std::string app_name;
  DatabaseConfig database;
  std::map<std::string, std::vector<int>> buckets;
  std::optional<std::string> environment;
};

template <>
struct vix::field_map<DatabaseConfig>
{
  static constexpr auto fields()
  {
    return vix::fields(
        vix::field("host", &DatabaseConfig::host),
        vix::field("port", &DatabaseConfig::port),
        vix::field("ssl", &DatabaseConfig::ssl));
  }
};

template <>
struct vix::field_map<AppConfig>
{
  static constexpr auto fields()
  {
    return vix::fields(
        vix::field("app_name", &AppConfig::app_name),
        vix::field("database", &AppConfig::database),
        vix::field("buckets", &AppConfig::buckets),
        vix::field("environment", &AppConfig::environment));
  }
};

int main()
{
  AppConfig cfg{
      "Softadastra",
      {"localhost", 5432, true},
      {{"low", {1, 2, 3}},
       {"high", {10, 20, 30}}},
      std::string{"production"}};

  vix::inspect_tree(cfg, "config");

  return 0;
}
