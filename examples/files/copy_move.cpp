/**
 * @file copy_move.cpp
 * @brief Demonstrates copying and renaming files with explicit policies.
 *
 * Creates or replaces files inside "vix-example-copy-move/" in the
 * current working directory. The example leaves them for inspection.
 */

#include <vix/files/Files.hpp>

#include <iostream>

int main()
{
  using namespace vix::files;

  auto directory = Path::from_utf8("vix-example-copy-move");
  auto source = Path::from_utf8("vix-example-copy-move/source.txt");
  auto copied = Path::from_utf8("vix-example-copy-move/copied.txt");
  auto renamed = Path::from_utf8("vix-example-copy-move/renamed.txt");

  if (!directory || !source || !copied || !renamed)
  {
    std::cerr << "Invalid example path.\n";
    return 1;
  }

  auto created = create_directories(directory.value());
  if (!created)
  {
    std::cerr << "Cannot create directory: " << created.error().message() << '\n';
    return 1;
  }

  // The destination policy is explicit: running the example again replaces it.
  auto written = write_utf8(source.value(), "Hello from Vix Files!\n",
                            WriteOptions::create_or_replace());
  if (!written)
  {
    std::cerr << "Cannot create source file: " << written.error().message() << '\n';
    return 1;
  }

  auto copied_result = copy_file(source.value(), copied.value(),
                                 CopyOptions::create_or_replace());
  if (!copied_result)
  {
    std::cerr << "Copy failed: " << copied_result.error().message() << '\n';
    return 1;
  }

  // Rename/move the copied file within the same filesystem.
  // This replaces an existing destination when the platform permits it.
  auto moved = rename_replace(copied.value(), renamed.value());
  if (!moved)
  {
    std::cerr << "Rename failed: " << moved.error().message() << '\n';
    return 1;
  }

  auto content = read_utf8(renamed.value(), ReadLimit{1024});
  if (!content)
  {
    std::cerr << "Read failed: " << content.error().message() << '\n';
    return 1;
  }

  std::cout << "Original: " << source.value().display() << '\n';
  std::cout << "Renamed copy: " << renamed.value().display() << '\n';
  std::cout << "Contents: " << content.value();

  // Copying and renaming are not general-purpose transactions: a failed
  // operation can leave a destination changed or partially written.
  return 0;
}
