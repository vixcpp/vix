#include <iostream>
#include <vix/fs/Fs.hpp>

int main()
{
  vix::fs::write_text("a.txt", "hello");

  auto copy = vix::fs::copy("a.txt", "b.txt");
  if (!copy)
  {
    std::cerr << "copy error\n";
    return 1;
  }

  auto move = vix::fs::move("b.txt", "c.txt");
  if (!move)
  {
    std::cerr << "move error\n";
    return 1;
  }

  auto rename = vix::fs::rename("c.txt", "final.txt");
  if (!rename)
  {
    std::cerr << "rename error\n";
    return 1;
  }

  std::cout << "done\n";

  vix::fs::remove("a.txt");
  vix::fs::remove("final.txt");

  return 0;
}
