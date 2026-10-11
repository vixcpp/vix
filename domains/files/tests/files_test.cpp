#include <vix/files/Files.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace
{
  vix::files::Path path(const std::string &value)
  {
    auto result = vix::files::Path::from_utf8(value);
    assert(result);
    return result.take();
  }
}

int main()
{
  namespace files = vix::files;
  const auto root = std::filesystem::temp_directory_path() / "vix_files_baseline_test";
  std::filesystem::remove_all(root);
  auto root_path = path(root.string());
  auto invalid_utf8_path = files::Path::from_utf8(std::string{"\xff", 1});
  assert(!invalid_utf8_path && invalid_utf8_path.error().identity() == files::errors::path_not_utf8);
  auto normalized = path("one/./two/../three").lexically_normal();
  assert(normalized.to_utf8().value() == "one/three");
  assert(path("one/../two") != path("two"));
  assert(files::create_directories(root_path));
  auto child = root_path.joined(path("payload.txt"));
  const std::string text = "hello";
  auto written = files::write_utf8(child, text, files::WriteOptions::create_new());
  assert(written);
  auto second_new = files::write_utf8(child, text, files::WriteOptions::create_new());
  assert(!second_new && second_new.error().identity() == files::errors::already_exists);
  auto read = files::read_utf8(child, files::ReadLimit{100});
  assert(read && read.value() == text);
  auto too_small = files::read_all(child, files::ReadLimit{4});
  assert(!too_small && too_small.error().identity() == files::errors::size_limit_exceeded);
  auto metadata = files::info(child);
  assert(metadata && metadata.value().type() == files::FileType::regular);
  auto opened = files::open(child, files::OpenOptions::existing(files::FileAccess::read));
  assert(opened);
  auto file = opened.take();
  std::byte buffer[8]{};
  assert(file.read_some(std::span<std::byte>{buffer, 0}).value() == 0);
  assert(file.seek(2, files::SeekOrigin::begin).value() == 2);
  assert(file.tell().value() == 2);
  assert(file.close());
  assert(file.close());

  auto append = files::open(child, files::OpenOptions::append_existing());
  assert(append);
  auto appender = append.take();
  assert(appender.seek(0, files::SeekOrigin::begin));
  const std::byte exclamation{static_cast<unsigned char>('!')};
  assert(appender.write_some(std::span<const std::byte>{&exclamation, 1}).value() == 1);
  assert(appender.close());
  assert(files::read_utf8(child, files::ReadLimit{100}).value() == "hello!");

  auto copied = root_path.joined(path("copied.txt"));
  assert(files::copy_file(child, copied, files::CopyOptions::create_new()));
  auto renamed = root_path.joined(path("renamed.txt"));
  assert(files::rename_replace(copied, renamed));

#if defined(__unix__) || defined(__APPLE__)
  auto linked = root_path.joined(path("entry-link"));
  assert(::symlink("payload.txt", linked.to_utf8().value().c_str()) == 0);
  assert(files::exists(linked).value());
  auto write_link = files::open(linked, files::OpenOptions::existing(files::FileAccess::write));
  assert(!write_link && write_link.error().identity() == files::errors::symlink_not_allowed);
#endif

  auto iterator = files::iterate(root_path);
  assert(iterator);
  std::size_t count = 0;
  while (true)
  {
    auto entry = iterator.value().next();
    assert(entry);
    if (!entry.value()) break;
    ++count;
  }
  assert(count >= 2);
  assert(!iterator.value().next().value().has_value());
  assert(iterator.value().close());
  assert(files::remove_tree(root_path, files::RemoveTreeOptions::require_existing()));
}
