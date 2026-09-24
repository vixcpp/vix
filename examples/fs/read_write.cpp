#include <iostream>
#include <vix/fs/Fs.hpp>

int main()
{
  const std::string path = "example.txt";

  auto write = vix::fs::write_text(path, "Hello Vix");

  if (!write)
  {
    std::cerr << "write error: " << write.error().message() << "\n";
    return 1;
  }

  auto read = vix::fs::read_text(path);

  if (!read)
  {
    std::cerr << "read error: " << read.error().message() << "\n";
    return 1;
  }

  std::cout << "content: " << read.value() << "\n";

  vix::fs::remove(path);

  return 0;
}
