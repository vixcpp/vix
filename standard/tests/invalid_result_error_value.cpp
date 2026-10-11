#include <vix/Result.hpp>

vix::Result<vix::Error> invalid_result{
    vix::Error{
        vix::ErrorIdentity::define<"vix.test">(1)}};

int main()
{
  return 0;
}
