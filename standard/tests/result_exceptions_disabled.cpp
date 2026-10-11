#include <vix/Result.hpp>

#include <memory>

inline constexpr vix::ErrorIdentity test_failure =
    vix::ErrorIdentity::define<"vix.test">(1);

int main()
{
  vix::Result<int> success{1};
  vix::Result<int> failure{vix::Error{test_failure}};
  vix::Result<std::unique_ptr<int>> owned{std::make_unique<int>(7)};
  auto value = owned.take();

  return success && !failure && value && *value == 7 ? 0 : 1;
}
