#include <iostream>
#include <vix/fs/Fs.hpp>

int main()
{
  auto cwd = vix::fs::current_path();

  if (!cwd)
  {
    std::cerr << "error: " << cwd.error().message() << "\n";
    return 1;
  }

  std::cout << "current directory: " << cwd.value() << "\n";

  auto tmp = vix::fs::temp_directory();

  if (!tmp)
  {
    std::cerr << "error: " << tmp.error().message() << "\n";
    return 1;
  }

  std::cout << "temp directory: " << tmp.value() << "\n";

  return 0;
}
