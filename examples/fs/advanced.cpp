#include <iostream>
#include <vix/fs/Fs.hpp>

int main()
{
  const std::string dir = "logs";

  auto ensured = vix::fs::ensure_directory(dir);
  if (!ensured)
  {
    std::cerr << "error: " << ensured.error().message() << "\n";
    return 1;
  }

  const std::string file = dir + "/app.log";

  vix::fs::append_file(file, "start\n");
  vix::fs::append_file(file, "running...\n");

  auto content = vix::fs::read_text(file);

  if (content)
  {
    std::cout << content.value() << "\n";
  }

  vix::fs::remove_all(dir);

  return 0;
}
