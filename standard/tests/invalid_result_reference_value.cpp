#include <vix/Result.hpp>

vix::Result<int &> invalid_result{*static_cast<int *>(nullptr)};

int main()
{
  return 0;
}
