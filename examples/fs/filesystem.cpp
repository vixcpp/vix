#include <iostream>
#include <vix/fs/Fs.hpp>

int main()
{
  const std::string dir = "demo_dir";

  auto created = vix::fs::create_directory(dir);

  if (!created)
  {
    std::cerr << "create error: " << created.error().message() << "\n";
    return 1;
  }

  std::cout << "created: " << created.value() << "\n";

  auto exists = vix::fs::exists(dir);

  if (exists && exists.value())
  {
    std::cout << "directory exists\n";
  }

  auto removed = vix::fs::remove(dir);

  if (!removed)
  {
    std::cerr << "remove error\n";
    return 1;
  }

  std::cout << "removed: " << removed.value() << "\n";

  return 0;
}
