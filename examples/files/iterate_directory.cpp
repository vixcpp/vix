/**
 * @file iterate_directory.cpp
 * @brief Demonstrates non-recursive directory iteration.
 *
 * Lists the current directory by default. Pass a UTF-8 path as argv[1]
 * to list another directory.
 */

#include <vix/files/Files.hpp>

#include <iostream>

int main(int argc, char *argv[])
{
  using namespace vix::files;

  const char *directory_name = argc > 1 ? argv[1] : ".";
  auto parsed = Path::from_utf8(directory_name);
  if (!parsed)
  {
    std::cerr << "Invalid directory path: "
              << parsed.error().message() << '\n';
    return 1;
  }

  auto directory = parsed.take();
  auto opened = iterate(directory);
  if (!opened)
  {
    std::cerr << "Cannot open directory: "
              << opened.error().message() << '\n';
    return 1;
  }

  auto iterator = opened.take();
  std::cout << "Entries in " << directory.display() << ":\n";

  while (true)
  {
    auto result = iterator.next();
    if (!result)
    {
      std::cerr << "Iteration failed: " << result.error().message() << '\n';
      return 1;
    }

    auto entry = result.take();
    if (!entry)
      break; // End of directory, not an error.

    // Type hints are not guaranteed to be authoritative metadata.
    const char *type = "unknown";
    switch (entry->type_hint())
    {
    case DirectoryEntryTypeHint::regular:
      type = "file";
      break;
    case DirectoryEntryTypeHint::directory:
      type = "directory";
      break;
    case DirectoryEntryTypeHint::symlink:
      type = "symlink";
      break;
    case DirectoryEntryTypeHint::other:
      type = "other";
      break;
    case DirectoryEntryTypeHint::unknown:
      break;
    }

    std::cout << "  [" << type << "] " << entry->path().display() << '\n';
  }

  auto closed = iterator.close();
  if (!closed)
  {
    std::cerr << "Close failed: " << closed.error().message() << '\n';
    return 1;
  }

  return 0;
}
