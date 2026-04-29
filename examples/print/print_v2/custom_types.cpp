#include <ostream>
#include <string>
#include <vector>

#include <vix/print.hpp>

struct Point
{
  int x;
  int y;
};

inline void vix_format(std::ostream &os, const Point &point)
{
  os << "Point{x=" << point.x << ", y=" << point.y << "}";
}

struct User
{
  std::string name;
  int age;
};

namespace vix
{
  template <>
  struct formatter<User>
  {
    static void format(std::ostream &os, const User &user)
    {
      os << "User{name=\"" << user.name << "\", age=" << user.age << "}";
    }
  };
} // namespace vix

int main()
{
  Point point{10, 20};
  User user{"Gaspard", 25};

  std::vector<Point> points{
      {1, 2},
      {3, 4},
      {5, 6}};

  std::vector<User> users{
      {"Alice", 21},
      {"Bob", 27},
      {"Charlie", 31}};

  vix::print(point);
  vix::print(user);

  vix::print("point:", point, "user:", user);

  vix::print(
      vix::options{.sep = " | "},
      "points", points,
      "users", users);

  return 0;
}
