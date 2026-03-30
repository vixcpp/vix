#include <vix/format.hpp>
#include <vix/print.hpp>

using namespace vix;

int main()
{
  try
  {
    print(format("Hello {", "world"));
  }
  catch (const format_error &e)
  {
    print("Error:", e.what());
  }

  try
  {
    print(format("{1}", 42));
  }
  catch (const format_error &e)
  {
    print("Error:", e.what());
  }

  return 0;
}
