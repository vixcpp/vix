#include <vix/ErrorIdentity.hpp>

inline constexpr auto invalid_identity =
    vix::ErrorIdentity::define<"">(1);

int main()
{
  return static_cast<int>(invalid_identity.code());
}
