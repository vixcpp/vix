#include <vix/files/Files.hpp>

#include <type_traits>

int main()
{
  using PathResult = vix::Result<vix::files::Path>;
  static_assert(!std::is_default_constructible_v<vix::files::Path>);
  static_assert(std::is_nothrow_move_constructible_v<vix::files::Path>);
  static_assert(std::is_nothrow_move_assignable_v<vix::files::Path>);
  static_assert(std::is_nothrow_move_constructible_v<vix::files::File>);
  static_assert(!std::is_move_assignable_v<vix::files::File>);
  (void)sizeof(PathResult);
}
