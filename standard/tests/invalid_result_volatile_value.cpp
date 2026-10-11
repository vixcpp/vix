#include <vix/Result.hpp>

vix::Result<volatile int> invalid_result{42};

int main()
{
  return 0;
}
