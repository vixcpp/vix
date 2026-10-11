#include <vix/Error.hpp>

namespace
{
  inline constexpr vix::ErrorIdentity allocation_probe =
      vix::ErrorIdentity::define<"vix.standard.test">(1);

  void exercise_error_allocation_surface()
  {
    vix::Error root{allocation_probe, "root"};
    vix::Error translated{allocation_probe, "translated", root};
    auto contextual = translated.with_context({"operation", "probe"});
    vix::Error copied{contextual};
    copied = root;
  }
} // namespace

int main()
{
  exercise_error_allocation_surface();
  return 0;
}
