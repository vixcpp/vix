#include <iostream>
#include <vix/fs/Fs.hpp>

int main()
{
  auto result = vix::fs::list_directory(".");

  if (!result)
  {
    std::cerr << "error: " << result.error().message() << "\n";
    return 1;
  }

  for (const auto &entry : result.value())
  {
    std::cout << entry.name;

    if (entry.type == vix::fs::FsEntryType::Directory)
    {
      std::cout << " [dir]";
    }

    std::cout << "\n";
  }

  return 0;
}
