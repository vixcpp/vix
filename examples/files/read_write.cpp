/**
 * @file read_write.cpp
 * @brief Demonstrates writing, reading, and inspecting a UTF-8 file.
 *
 * Creates or replaces "vix-example-hello.txt" in the working directory.
 */

#include <vix/files/Files.hpp>

#include <iostream>
#include <string>

int main()
{
  using namespace vix::files;

  auto parsed = Path::from_utf8("vix-example-hello.txt");
  if (!parsed)
  {
    std::cerr << "Invalid path: " << parsed.error().message() << '\n';
    return 1;
  }

  auto path = parsed.take();

  // Explicitly choose what happens if the file already exists.
  const std::string text = "Hello from Vix Files!\n";
  auto written = write_utf8(path, text, WriteOptions::create_or_replace());
  if (!written)
  {
    std::cerr << "Write failed: " << written.error().message() << '\n';
    return 1;
  }

  // ReadLimit prevents accidentally loading an unbounded amount of data.
  auto read = read_utf8(path, ReadLimit{1024});
  if (!read)
  {
    std::cerr << "Read failed: " << read.error().message() << '\n';
    return 1;
  }

  std::cout << "File: " << path.display() << '\n';
  std::cout << "Contents: " << read.value();

  // Metadata is a snapshot; its size may be unavailable.
  auto metadata = info(path);
  if (!metadata)
  {
    std::cerr << "Metadata failed: " << metadata.error().message() << '\n';
    return 1;
  }

  if (metadata.value().size())
    std::cout << "Size: " << *metadata.value().size() << " bytes\n";

  return 0;
}
