#include <vix/print.hpp>

#include <ostream>
#include <string>
#include <vector>

namespace app
{

  struct Point
  {
    int x{};
    int y{};
  };

  inline void vix_format(std::ostream &os, const Point &p)
  {
    os << "Point{x=" << p.x << ", y=" << p.y << "}";
  }

  struct User
  {
    std::string name;
    int age{};
  };

  inline std::ostream &operator<<(std::ostream &os, const User &user)
  {
    return os << "User(name=" << user.name << ", age=" << user.age << ")";
  }

  struct Color
  {
    int r{};
    int g{};
    int b{};
  };

} // namespace app

template <>
struct vix::formatter<app::Color>
{
  static void format(std::ostream &os, const app::Color &c)
  {
    os << "Color(" << c.r << ", " << c.g << ", " << c.b << ")";
  }
};

int main()
{
  vix::print_header("Custom types");

  const app::Point p{10, 20};
  const app::User user{"Gaspard", 25};
  const app::Color color{255, 140, 0};

  vix::print(p);
  vix::print(user);
  vix::print(color);

  vix::print(std::vector<app::Point>{{0, 0}, {1, 2}, {3, 4}});
  vix::print(std::vector<app::User>{{"Alice", 21}, {"Bob", 30}});

  return 0;
}
