#include <vix/ErrorIdentity.hpp>

inline constexpr auto invalid_identity =
    vix::ErrorIdentity::define<"vix.test">(0);

int main()
{
  return static_cast<int>(invalid_identity.code());
}
