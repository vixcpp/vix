#include <vix/inspect.hpp>

#include <optional>
#include <string>
#include <vector>

namespace app
{
  struct Vec2
  {
    float x;
    float y;
  };

  inline void vix_inspect(vix::inspect_context &ctx, const Vec2 &value)
  {
    ctx.os << "Vec2(x=" << value.x << ", y=" << value.y << ")";
  }

  struct Vec3
  {
    float x;
    float y;
    float z;
  };

  struct Profile
  {
    std::string name;
    int age;
    std::vector<std::string> skills;
    std::optional<double> score;
  };

  struct Metric
  {
    std::string label;
    double value;
  };
}

template <>
struct vix::field_map<app::Vec3>
{
  static constexpr auto fields()
  {
    return vix::fields(
        vix::field("x", &app::Vec3::x),
        vix::field("y", &app::Vec3::y),
        vix::field("z", &app::Vec3::z));
  }
};

template <>
struct vix::field_map<app::Profile>
{
  static constexpr auto fields()
  {
    return vix::fields(
        vix::field("name", &app::Profile::name),
        vix::field("age", &app::Profile::age),
        vix::field("skills", &app::Profile::skills),
        vix::field("score", &app::Profile::score));
  }
};

template <>
struct vix::inspector<app::Metric>
{
  static void inspect(vix::inspect_context &ctx, const app::Metric &value)
  {
    ctx.os << value.label << '=' << value.value;
    if (ctx.opts.show_type)
    {
      ctx.os << " [Metric]";
    }
  }
};

int main()
{
  app::Vec2 p2{1.5f, -2.0f};
  app::Vec3 p3{1.0f, 2.0f, 3.0f};
  app::Metric metric{"accuracy", 0.987};
  app::Profile profile{
      "Gaspard",
      24,
      {"C++", "Node.js", "Systems"},
      91.5};

  vix::inspect(p2);
  vix::inspect(p3);
  vix::inspect(metric);
  vix::inspect(profile);

  vix::inspect_report(profile);

  return 0;
}
