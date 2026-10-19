#include <vix/Result.hpp>

int main()
{
  return static_cast<bool>(vix::Result<void>::success()) ? 0 : 1;
}
