#include <iostream>
#include <vix/error/Exception.hpp>

using namespace vix::error;

void open_file()
{
  throw Exception(
      ErrorCode::IoError,
      ErrorCategory::io(),
      "failed to open file");
}

int main()
{
  try
  {
    open_file();
  }
  catch (const Exception &e)
  {
    std::cout << "Caught: " << e.what() << "\n";
    std::cout << "Category: " << e.error().category().name() << "\n";
  }
}
