#include <vix/inspect.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct BuildInfo
{
  std::string project;
  int threads;
  bool release;
};

template <>
struct vix::field_map<BuildInfo>
{
  static constexpr auto fields()
  {
    return vix::fields(
        vix::field("project", &BuildInfo::project),
        vix::field("threads", &BuildInfo::threads),
        vix::field("release", &BuildInfo::release));
  }
};

int main()
{
  BuildInfo info{"vix", 8, true};

  vix::inspect_type<int>();
  vix::inspect_type<std::vector<std::string>>();
  vix::inspect_type<BuildInfo>();

  vix::inspect_meta(info);

  vix::inspect_paths<
      int,
      std::string,
      std::vector<int>,
      std::optional<int>,
      std::variant<int, std::string>,
      std::shared_ptr<int>,
      std::filesystem::path,
      BuildInfo>();

  vix::inspect_diff(42, 43, "expected", "actual");
  vix::inspect_check("sum check", 6, 1 + 2 + 3);
  vix::inspect_check("bad check", 10, 2 * 3);

  return 0;
}
